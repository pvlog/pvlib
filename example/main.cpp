/*
 *   Pvlib - Connection interface
 *
 *   Copyright (C) 2011
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/


#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <getopt.h>

#include <pvlib.h>

const static char *levelName[] = { "ERROR", "INFO", "WARNING", "DEBUG", "TRACE" };
#define MAX_LOG_MODULES 20

static void print_usage() {
	printf(
			"Usage: pvlib [options] MAC PASSWORD\n"
			"Options:\n"
			"-d <module> modules logging should be enabled for.\n"
			"-l <severity> log severity can be error, warning, info, debug, trace.\n"
			"-s read spot data."
			"-e read event archive"
			"-y read day archive"
			"-i read inverter info"
			"\n"
			"Example: pvlib \"00:11:22:33:44:55\" \"0000\"\n");
}

static void log_callback(const char *module, const char *filename, int line, pvlib_log_level level, const char *message) {
	time_t curTime = time(nullptr);
	char buffer[30];

	tm* timeinfo = localtime(&curTime);
	strftime(buffer, 30, "%Y-%m-%d %T", timeinfo);

	printf("%s[%s %s:%d] %s\n", levelName[(int)level], buffer, filename, line, message);
}

static int getInverterInfo(pvlib_plant* plant, uint32_t inv_handle) {
	pvlib_status *status = pvlib_alloc_status();
	pvlib_inverter_info *inverter_info = pvlib_alloc_inverter_info();
	pvlib_stats *stats = pvlib_alloc_stats();
	if (stats == nullptr || status == nullptr || inverter_info == nullptr) {
		fprintf(stderr, "Out of memory!");
		return -1;
	}

	// Read inverter status
	if (pvlib_get_status(plant, inv_handle, status) < 0) {
		fprintf(stderr, "get status failed!\n");
		pvlib_free_inverter_info(inverter_info);
		pvlib_free_status(status);
		return -1;
	}

	// Read inverter info
	if (pvlib_get_inverter_info(plant, inv_handle, inverter_info) < 0) {
		fprintf(stderr, "get info failed!\n");
		pvlib_free_inverter_info(inverter_info);
		pvlib_free_status(status);
		return -1;
	}
	printf("Manufacture: %s\n", inverter_info->manufacture);
	printf("Type: %s\n", inverter_info->type);
	printf("Name: %s\n", inverter_info->name);
	printf("Firmware: %s\n", inverter_info->firmware_version);
	printf("status: %d %d\n", status->status, status->number);

	// Read inverter stats
	if (pvlib_get_stats(plant, inv_handle, stats) < 0) {
		fprintf(stderr, "get stats failed!\n");
		pvlib_free_inverter_info(inverter_info);
		pvlib_free_status(status);
		return -1;
	}

	pvlib_free_inverter_info(inverter_info);
	pvlib_free_status(status);

	return 0;
}

static int getSpotData(pvlib_plant* plant, uint32_t inv_handle) {
	pvlib_ac *ac = pvlib_alloc_ac();
	pvlib_dc *dc = pvlib_alloc_dc();
	if (ac == nullptr || dc == nullptr) {
		fprintf(stderr, "Out of memory!");
		return EXIT_FAILURE;
	}

	// Read inverter ac data
	if (pvlib_get_ac_values(plant, inv_handle, ac) < 0) {
		fprintf(stderr, "get live values failed!\n");
		pvlib_free_ac(ac);
		pvlib_free_dc(dc);
		return -1;
	}

	// Read inverter dc data
	if (pvlib_get_dc_values(plant, inv_handle, dc) < 0) {
		fprintf(stderr, "get live values failed!\n");
		pvlib_free_ac(ac);
		pvlib_free_dc(dc);
		return -1;
	}

	pvlib_free_ac(ac);
	pvlib_free_dc(dc);

	return 0;
}

static int getDayArchive(pvlib_plant* plant, uint32_t inv_handle) {
	time_t to = time(0);
	time_t from = to - 24 * 60 * 60 * 7;

	int days = 0;
	pvlib_day_yield *dayYield;
	if ((days = pvlib_get_day_yield(plant, inv_handle, from, to, &dayYield)) < 0) {
		fprintf(stderr, "get day yield failed\n");
		return -1;

	}

	for (int i = 0; i < days; ++i) {
		printf("%s: %d\n", ctime(&dayYield[i].date), (int32_t)dayYield[i].dayYield);
	}

	free(dayYield);

	return 0;
}

static int getEventArchive(pvlib_plant* plant, uint32_t inv_handle) {
	pvlib_event *events;
	int eventNum;
	time_t to = time(0);
	time_t from = to - 24 * 60 * 60 * 7;
	if ((eventNum = pvlib_get_events(plant, inv_handle, from, to, &events)) < 0) {
		fprintf(stderr, "get day events failed\n");
		return -1;

	}

	for (int i = 0; i < eventNum; ++i) {
		printf("%s: %s (%d)\n", ctime(&events[i].time), events[i].message, events[i].value);
	}

	free(events);

	return 0;
}

int main(int argc, char **argv) {
	pvlib_plant *plant;

	int inv_num;
	uint32_t inv_handle;

	int found;
	int i;

	int con_num;
	uint32_t con_handles[10];
	int prot_num;
	uint32_t prot_handles[10];

	uint32_t con;
	uint32_t prot;
	bool readSpotData = false;
	bool readEventArchive = false;
	bool readDayArchive = false;
	bool readInverterInfo = false;


	const char *modules[MAX_LOG_MODULES];
	memset(modules, 0, sizeof(modules));

	int log_modules = 0;
	int c;
	pvlib_log_level log_level = PVLIB_LOG_WARNING;
	while ((c = getopt(argc, argv, "d:l:")) != -1) {
		switch (c) {
		case 'd':
			modules[log_modules++] = optarg;
			break;
		case 'l':
			if (strcmp(optarg, "error") == 0) {
				log_level = PVLIB_LOG_ERROR;
			} else if (strcmp(optarg, "warning") == 0) {
				log_level = PVLIB_LOG_WARNING;
			} else if (strcmp(optarg, "info") == 0) {
				log_level = PVLIB_LOG_INFO;
			} else if (strcmp(optarg, "debug") == 0) {
				log_level = PVLIB_LOG_DEBUG;
			} else if (strcmp(optarg, "trace") == 0) {
				log_level = PVLIB_LOG_TRACE;
			} else {
				printf("Invalid debug level!");
				print_usage();
				exit(EXIT_FAILURE);
			}
			break;
		case 's':
			readSpotData = true;
			break;
		case 'e':
			readEventArchive = true;
			break;
		case 'y':
			readDayArchive = true;
			break;
		case 'i':
			readInverterInfo = true;
			break;
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	if (argc - optind < 2) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	pvlib_init(log_callback, modules, log_level);

	con_num = pvlib_connections(con_handles, 10);

	found = 0;
	for (i = 0; i < con_num; i++) {
		if (strcmp(pvlib_connection_name(con_handles[i]), "rfcomm") == 0) {
			found = 1;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "connection rfcomm not available!\n");
		return EXIT_FAILURE;
	}

	con = con_handles[i];

	prot_num = pvlib_protocols(prot_handles, 10);

	found = 0;
	for (i = 0; i < prot_num; i++) {
		if (strcmp(pvlib_protocol_name(prot_handles[i]), "smadata2plus") == 0) {
			found = 1;
			break;
		}
	}

	if (!found) {
		fprintf(stderr, "protocol smadata2plus not available!\n");
		return EXIT_FAILURE;
	}

	prot = prot_handles[i];

	plant = pvlib_open(con, prot, NULL, NULL);
	if (plant == NULL) {
		fprintf(stderr, "Failed opening plant!\n");
		return EXIT_FAILURE;
	}

	if (pvlib_connect(plant, argv[optind], argv[optind + 1], NULL, NULL) < 0) {
		fprintf(stderr, "Failed connection with plant!\n");
		return EXIT_FAILURE;
	}

	inv_num = pvlib_num_string_inverter(plant);

	if (inv_num <= 0) {
		fprintf(stderr, "no inverters found!\n");
		return EXIT_FAILURE;
	}

	if (inv_num > 1) {
		fprintf(stderr, "more than %d inverter, but only 1 is currently supported!\n",
				inv_num);
		return EXIT_FAILURE;
	}

	if (pvlib_device_handles(plant, &inv_handle, 1) != 1) {
		fprintf(stderr, "Error getting inverter handle\n");
		return -1;
	}

	if (readInverterInfo) {
		if (getInverterInfo(plant, inv_handle) < 0) {
			return EXIT_FAILURE;
		}
	}


	if (readSpotData) {
		if (getSpotData(plant, inv_handle) < 0) {
			return EXIT_FAILURE;
		}
	}

	if (readDayArchive) {
		if (getDayArchive(plant, inv_handle) < 0) {
			return EXIT_FAILURE;
		}

	}

	if (readEventArchive) {
		if (getEventArchive(plant, inv_handle) < 0) {
			return EXIT_FAILURE;
		}
	}

	// Close pvlib
	pvlib_close(plant);
	pvlib_shutdown();

	return 0;
}
