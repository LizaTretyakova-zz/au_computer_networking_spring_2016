#pragma once

enum cmd_code: uint8_t {
	cd = 0,
	ls = 1,
	get = 2,
	put = 3,
	del = 4
};

enum status_code: uint8_t {
	ok = 1,
	fail = 0
};

struct request {
	cmd_code cmd;
	char* target_path;
	char* data;
	uint32_t data_size;

	request(cmd_code c, char* tp, char* d, uint32_t ds):
		cmd(c), target_path(tp), data(d), data_size(ds) {}
};

struct response {
	status_code status;
	char* data;
	uint32_t data_size;

	response(status_code s = fail, char* d = NULL, uint32_t ds = 0):
		status(s), data(d), data_size(ds) {}
};
