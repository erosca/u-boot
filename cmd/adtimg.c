// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2018 Linaro Ltd.
 * Sam Protsenko <semen.protsenko@linaro.org>
 * Eugeniu Rosca <rosca.eugeniu@gmail.com>
 */

#include <env.h>
#include <image-android-dt.h>
#include <common.h>

/*
 * Current/working DTB/DTBO Android image address.
 * Similar to 'working_fdt' variable in 'fdt' command.
 */
static ulong working_img;

static int do_adtimg_addr(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	char *endp;
	ulong hdr_addr;

	if (argc != 2)
		return CMD_RET_USAGE;

	hdr_addr = simple_strtoul(argv[1], &endp, 16);
	if (*endp != '\0') {
		printf("Error: Wrong image address '%s'\n", argv[1]);
		return CMD_RET_FAILURE;
	}

	/*
	 * Allow users to set an address prior to copying the DTB/DTBO
	 * image to that same address, i.e. skip header verification.
	 */

	working_img = hdr_addr;
	return CMD_RET_SUCCESS;
}

static int adtimg_check_working_img(void)
{
	if (!working_img) {
		puts("Error: Please, call 'adtimg addr <addr>'. Aborting!\n");
		return CMD_RET_FAILURE;
	}

	if (!android_dt_check_header(working_img)) {
		printf("Error: Invalid image header at 0x%lx\n", working_img);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_adtimg_dump(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	if (argc != 1)
		return CMD_RET_USAGE;

	if (adtimg_check_working_img() != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	android_dt_print_contents(working_img);

	return CMD_RET_SUCCESS;
}

static int adtimg_get_dt_by_index(int argc, char * const argv[])
{
	ulong addr;
	u32 size, index;
	char *endp;

	if (argc < 1) {
		puts("Error: Option 'get dt index' requires an argument\n");
		return CMD_RET_FAILURE;
	}

	if (argc > 3) {
		puts("Error: Too many arguments for 'get dt index'\n");
		return CMD_RET_FAILURE;
	}

	index = simple_strtoul(argv[0], &endp, 0);
	if (*endp != '\0') {
		printf("Error: Wrong index value '%s'\n", argv[0]);
		return CMD_RET_FAILURE;
	}

	if (!android_dt_get_fdt_by_index(working_img, index, &addr, &size))
		return CMD_RET_FAILURE;

	if (argc == 3) {
		env_set_hex(argv[1], addr);
		env_set_hex(argv[2], size);
	} else {
		printf("0x%lx, 0x%x (dec:%d)\n", addr, size, size);
	}

	return CMD_RET_SUCCESS;
}

static int adtimg_get_dt(int argc, char * const argv[])
{
	const char *param;

	if (argc < 1) {
		puts("Error: Option 'get dt' requires an argument\n");
		return CMD_RET_FAILURE;
	}

	param = argv[0];
	argc--;
	argv++;

	if (!strcmp(param, "index"))
		return adtimg_get_dt_by_index(argc, argv);

	printf("Error: Wrong option 'get dt %s'\n", param);
	return CMD_RET_FAILURE;
}

static int do_adtimg_get(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	const char *param;

	if (argc < 2) {
		puts("Error: Option 'get' requires an argument\n");
		return CMD_RET_FAILURE;
	}

	if (adtimg_check_working_img() != CMD_RET_SUCCESS)
		return CMD_RET_FAILURE;

	param = argv[1];
	argc -= 2;
	argv += 2;

	if (!strcmp(param, "dt"))
		return adtimg_get_dt(argc, argv);

	printf("Error: Wrong option 'get %s'\n", param);
	return CMD_RET_FAILURE;
}

static cmd_tbl_t cmd_adtimg_sub[] = {
	U_BOOT_CMD_MKENT(addr, CONFIG_SYS_MAXARGS, 1, do_adtimg_addr, "", ""),
	U_BOOT_CMD_MKENT(dump, CONFIG_SYS_MAXARGS, 1, do_adtimg_dump, "", ""),
	U_BOOT_CMD_MKENT(get, CONFIG_SYS_MAXARGS, 1, do_adtimg_get, "", ""),
};

static int do_adtimg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_adtimg_sub, ARRAY_SIZE(cmd_adtimg_sub));

	/* Strip off leading 'adtimg' command argument */
	argc--;
	argv++;

	if (!cp || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp))
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	adtimg, CONFIG_SYS_MAXARGS, 0, do_adtimg,
	"manipulate dtb/dtbo Android image",
	"addr <addr>\t\t\t- Set image location to <addr>\n"
	"adtimg dump\t\t\t\t- Print out image contents\n"
	"adtimg get dt index <index> [avar svar]\t- Get DT address and size by index\n"
	"\n"
	"Legend:\n"
	"  - <addr>: DTB/DTBO image address (hex) in RAM\n"
	"  - <index>: index (hex/dec) of desired DT in the image\n"
	"  - <avar>: variable name to contain DT address (hex)\n"
	"  - <svar>: variable name to contain DT size (hex)"
);
