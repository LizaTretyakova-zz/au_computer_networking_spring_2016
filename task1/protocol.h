#pragma once

enum cmd_code: uint8_t {
	cd = 0,
	ls = 1,
	get = 2,
	put = 3,
	del = 4
}

enum status_code: uint8_t {
	ok = 1,
	failed = 0
}

struct request {
	cmd_code cmd;
	char* target_path;
	char* data;
	uint32_t data_size;
}

struct response {
	status_code status;
	char* data;
	uint32_t data_size;
}
