/*
 ============================================================================
 Name        : seektime.c
 Author      : Dave Shadoff
 Version     :
 Copyright   : (C) 2018 Dave Shadoff
 Description : Program to determine seek time, based on start and end sector numbers
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct sector_group {
		int		sec_per_revolution;
		int		sec_start;
		int		sec_end;
		float	rotation_ms;
		float	rotation_vsync;
} sector_group;

#define NUM_SECTOR_GROUPS	14

sector_group sector_list[NUM_SECTOR_GROUPS] = {
	{ 10,	0,				12572,	133.47,	8.00 },
	{ 11,	12573,	30244,	146.82,	8.81 },		// Except for the first and last groups,
	{ 12,	30245,	49523,	160.17,	9.61 },		// there are 1606.5 tracks in each range
	{ 13,	49524,	70408,	173.51,	10.41 },
	{ 14,	70409,	92900,	186.86,	11.21 },
	{ 15,	92901,	116998,	200.21,	12.01 },
	{ 16,	116999,	142703,	213.56,	12.81 },
	{ 17,	142704,	170014,	226.90,	13.61 },
	{ 18,	170015,	198932,	240.25,	14.42 },
	{ 19,	198933,	229456,	253.60,	15.22 },
	{ 20,	229457,	261587,	266.95,	16.02 },
	{ 21,	261588,	295324,	280.29,	16.82 },
	{ 22,	295325,	330668,	293.64,	17.62 },
	{ 23,	330669,	333012,	306.99,	18.42 }
};




int get_int(char * in_string);
int find_group(int sector_num);


int main(int argc, char * argv[]) {

int start_sector;
int start_index;
int target_sector;
int target_index;

float track_difference;
float vsync_count = 0;
float milliseconds = 0;


	if (argc != 3) {
		printf("%d arguments\n", argc);
		printf("Usage: seektime <start_sector> <target_sector>\n");
		exit(1);
	}

	start_sector = get_int(argv[1]);
	if (start_sector == -1) {
		printf("Invalid character in start_sector\n");
		exit(1);
	}
	if ((start_sector < sector_list[0].sec_start) || (start_sector > sector_list[NUM_SECTOR_GROUPS-1].sec_end))
	{
		printf("Bad start sector # %d\n", start_sector);
		exit(1);
	}

	target_sector = get_int(argv[2]);
	if (target_sector == -1) {
		printf("Invalid character in target_sector\n");
		exit(1);
	}
	if ((target_sector < sector_list[0].sec_start) || (target_sector > sector_list[NUM_SECTOR_GROUPS-1].sec_end))
	{
		printf("Bad target sector # %d\n", target_sector);
		exit(1);
	}

	// First, we identify which group the start and end are in
	start_index = find_group(start_sector);
	target_index = find_group(target_sector);

	// Now we find the track difference
	//
	// Note: except for the first and last sector groups, all groups are 1606.48 tracks per group.
	//
	if (target_index == start_index)
	{
		track_difference = (float)(abs(target_sector - start_sector) / sector_list[target_index].sec_per_revolution);
	}
	else if (target_index > start_index)
	{
		track_difference = (sector_list[start_index].sec_end - start_sector) / sector_list[start_index].sec_per_revolution;
		track_difference += (target_sector - sector_list[target_index].sec_start) / sector_list[target_index].sec_per_revolution;
		track_difference += (1606.48 * (target_index - start_index - 1));
	}
	else // start_index > target_index
	{
		track_difference = (start_sector - sector_list[start_index].sec_start) / sector_list[start_index].sec_per_revolution;
		track_difference += (sector_list[target_index].sec_end - target_sector) / sector_list[target_index].sec_per_revolution;
		track_difference += (1606.48 * (start_index - target_index - 1));
	}

	// Now, we use the algorithm to determine how long to wait
	if (abs(target_sector - start_sector) < 2)
	{
		vsync_count = 3;
		milliseconds = (3 * 1000 / 60);
	}
	if (abs(target_sector - start_sector) < 5)
	{
		vsync_count = 9 + (float)(sector_list[target_index].rotation_vsync / 2);
		milliseconds = (9 * 1000 / 60) + (float)(sector_list[target_index].rotation_ms / 2);
	}
	else if (track_difference <= 80)
	{
		vsync_count = 16 + (float)(sector_list[target_index].rotation_vsync / 2);
		milliseconds = (16 * 1000 / 60) + (float)(sector_list[target_index].rotation_ms / 2);
	}
	else if (track_difference <= 160)
	{
		vsync_count = 22 + (float)(sector_list[target_index].rotation_vsync / 2);
		milliseconds = (22 * 1000 / 60) + (float)(sector_list[target_index].rotation_ms / 2);
	}
	else if (track_difference <= 644)
	{
		vsync_count = 22 + (float)(sector_list[target_index].rotation_vsync / 2) + (float)((track_difference - 161) / 80);
		milliseconds = (22 * 1000 / 60) + (float)(sector_list[target_index].rotation_ms / 2) + (float)((track_difference - 161) * 16.66 / 80);
	}
	else
	{
		vsync_count = 36 + (float)((track_difference - 644) / 195);
		milliseconds = (36 * 1000 / 60) + (float)((track_difference - 644) * 16.66 / 195);
	}

	printf("From sector %d to sector %d:\n", start_sector, target_sector);
	printf("Time = %.2f VSYNCs; %.2f milliseconds\n", vsync_count, milliseconds);

	return EXIT_SUCCESS;
}

int find_group(int sector_num)
{
	int i;
	int group_index = 0;

	for (i = 0; i < NUM_SECTOR_GROUPS; i++)
	{
		if ((sector_num >= sector_list[i].sec_start) && (sector_num <= sector_list[i].sec_end))
		{
			group_index = i;
			break;
		}
	}
	return group_index;
}

/*
 *  This function just takes a string and changes it to an integer.
 *  Hexadecimal is also OK (prefixed by '0x')
 */

int get_int(char * in_string)
{
	int a = 0;
	int base = 10;
	int offset = 0;
	int iter;
	int char_val;

	if (in_string[0] == '$') {
		base = 16;
		offset = 1;
	}
	else if ((in_string[0] == '0') && (in_string[1] == 'x')) {
		base = 16;
		offset = 2;
	}

	iter = offset;
	while (iter < strlen(in_string)) {
		char_val = 0;

		if ((in_string[iter] >= '0') && (in_string[iter] <= '9')) {
			char_val = (in_string[iter] - '0');
		}
		else if ( (base == 16) &&  ((in_string[iter] >= 'A') && (in_string[iter] <= 'F'))) {
			char_val = (in_string[iter] - 'A' + 10);
		}
		else if ( (base == 16) &&  ((in_string[iter] >= 'a') && (in_string[iter] <= 'f'))) {
			char_val = (in_string[iter] - 'a' + 10);
		}
		else {
			a = -1;		/* Invalid Character */
			break;
		}

		a = (a * base) + char_val;
		iter++;
	}

	return(a);
}
