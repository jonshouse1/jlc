// jlc_isdaylight.c

#include <stdio.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

#define TRUE    1
#define FALSE   0


// Table of sunrise and sunset times by month and day of the month. 
// One years worth of data
// sunrise and sunset times are UTC
char risesettable[367][64]= {
		"Month	Day	Sunrise	Sunset	Daylength",
		"1 	1 	08:20 	16:01 	07:41",
		"1 	2 	08:20 	16:02 	07:42",
		"1 	3 	08:20 	16:03 	07:43",
		"1 	4 	08:20 	16:04 	07:44",
		"1 	5 	08:19 	16:05 	07:46",
		"1 	6 	08:19 	16:07 	07:48",
		"1 	7 	08:18 	16:08 	07:50",
		"1 	8 	08:18 	16:09 	07:51",
		"1 	9 	08:17 	16:11 	07:54",
		"1 	10 	08:17 	16:12 	07:55",
		"1 	11 	08:16 	16:13 	07:57",
		"1 	12 	08:16 	16:15 	07:59",
		"1 	13 	08:15 	16:17 	08:02",
		"1 	14 	08:14 	16:18 	08:04",
		"1 	15 	08:13 	16:20 	08:07",
		"1 	16 	08:12 	16:21 	08:09",
		"1 	17 	08:11 	16:23 	08:12",
		"1 	18 	08:10 	16:24 	08:14",
		"1 	19 	08:09 	16:26 	08:17",
		"1 	20 	08:08 	16:28 	08:20",
		"1 	21 	08:07 	16:30 	08:23",
		"1 	22 	08:06 	16:31 	08:25",
		"1 	23 	08:05 	16:33 	08:28",
		"1 	24 	08:04 	16:35 	08:31",
		"1 	25 	08:02 	16:37 	08:35",
		"1 	26 	08:01 	16:39 	08:38",
		"1 	27 	07:59 	16:40 	08:41",
		"1 	28 	07:58 	16:42 	08:44",
		"1 	29 	07:57 	16:44 	08:47",
		"1 	30 	07:55 	16:46 	08:51",
		"1 	31 	07:54 	16:48 	08:54",
		"2 	1 	07:52 	16:50 	08:58",
		"2 	2 	07:50 	16:52 	09:02",
		"2 	3 	07:49 	16:53 	09:04",
		"2 	4 	07:47 	16:55 	09:08",
		"2 	5 	07:45 	16:57 	09:12",
		"2 	6 	07:44 	16:59 	09:15",
		"2 	7 	07:42 	17:01 	09:19",
		"2 	8 	07:40 	17:03 	09:23",
		"2 	9 	07:38 	17:05 	09:27",
		"2 	10 	07:36 	17:07 	09:31",
		"2 	11 	07:35 	17:09 	09:34",
		"2 	12 	07:33 	17:11 	09:38",
		"2 	13 	07:31 	17:13 	09:42",
		"2 	14 	07:29 	17:15 	09:46",
		"2 	15 	07:27 	17:16 	09:49",
		"2 	16 	07:25 	17:18 	09:53",
		"2 	17 	07:23 	17:20 	09:57",
		"2 	18 	07:21 	17:22 	10:01",
		"2 	19 	07:19 	17:24 	10:05",
		"2 	20 	07:17 	17:26 	10:09",
		"2 	21 	07:15 	17:28 	10:13",
		"2 	22 	07:13 	17:30 	10:17",
		"2 	23 	07:10 	17:32 	10:22",
		"2 	24 	07:08 	17:33 	10:25",
		"2 	25 	07:06 	17:35 	10:29",
		"2 	26 	07:04 	17:37 	10:33",
		"2 	27 	07:02 	17:39 	10:37",
		"2 	28 	07:00 	17:41 	10:41",
		"2 	29 	07:00 	17:41 	10:41",
		"3 	1 	06:57 	17:43 	10:46",
		"3 	2 	06:55 	17:45 	10:50",
		"3 	3 	06:53 	17:46 	10:53",
		"3 	4 	06:51 	17:48 	10:57",
		"3 	5 	06:48 	17:50 	11:02",
		"3 	6 	06:46 	17:52 	11:06",
		"3 	7 	06:44 	17:54 	11:10",
		"3 	8 	06:42 	17:56 	11:14",
		"3 	9 	06:39 	17:57 	11:18",
		"3 	10 	06:37 	17:59 	11:22",
		"3 	11 	06:35 	18:01 	11:26",
		"3 	12 	06:32 	18:03 	11:31",
		"3 	13 	06:30 	18:05 	11:35",
		"3 	14 	06:28 	18:06 	11:38",
		"3 	15 	06:25 	18:08 	11:43",
		"3 	16 	06:23 	18:10 	11:47",
		"3 	17 	06:21 	18:12 	11:51",
		"3 	18 	06:18 	18:13 	11:55",
		"3 	19 	06:16 	18:15 	11:59",
		"3 	20 	06:14 	18:17 	12:03",
		"3 	21 	06:11 	18:19 	12:08",
		"3 	22 	06:09 	18:21 	12:12",
		"3 	23 	06:07 	18:22 	12:15",
		"3 	24 	06:04 	18:24 	12:20",
		"3 	25 	06:02 	18:26 	12:24",
		"3 	26 	06:00 	18:28 	12:28",
		"3 	27 	05:57 	18:29 	12:32",
		"3 	28 	05:55 	18:31 	12:36",
		"3 	29 	05:53 	18:33 	12:40",
		"3 	30 	05:50 	18:35 	12:45",
		"3 	31 	05:48 	18:36 	12:48",
		"4 	1 	05:46 	18:38 	12:52",
		"4 	2 	05:43 	18:40 	12:57",
		"4 	3 	05:41 	18:42 	13:01",
		"4 	4 	05:38 	18:43 	13:05",
		"4 	5 	05:36 	18:45 	13:09",
		"4 	6 	05:34 	18:47 	13:13",
		"4 	7 	05:31 	18:48 	13:17",
		"4 	8 	05:29 	18:50 	13:21",
		"4 	9 	05:27 	18:52 	13:25",
		"4 	10 	05:25 	18:54 	13:29",
		"4 	11 	05:22 	18:55 	13:33",
		"4 	12 	05:20 	18:57 	13:37",
		"4 	13 	05:18 	18:59 	13:41",
		"4 	14 	05:16 	19:01 	13:45",
		"4 	15 	05:13 	19:02 	13:49",
		"4 	16 	05:11 	19:04 	13:53",
		"4 	17 	05:09 	19:06 	13:57",
		"4 	18 	05:07 	19:08 	14:01",
		"4 	19 	05:05 	19:09 	14:04",
		"4 	20 	05:02 	19:11 	14:09",
		"4 	21 	05:00 	19:13 	14:13",
		"4 	22 	04:58 	19:15 	14:17",
		"4 	23 	04:56 	19:16 	14:20",
		"4 	24 	04:54 	19:18 	14:24",
		"4 	25 	04:52 	19:20 	14:28",
		"4 	26 	04:50 	19:22 	14:32",
		"4 	27 	04:48 	19:23 	14:35",
		"4 	28 	04:46 	19:25 	14:39",
		"4 	29 	04:44 	19:27 	14:43",
		"4 	30 	04:42 	19:28 	14:46",
		"5 	1 	04:40 	19:30 	14:50",
		"5 	2 	04:38 	19:32 	14:54",
		"5 	3 	04:36 	19:33 	14:57",
		"5 	4 	04:34 	19:35 	15:01",
		"5 	5 	04:32 	19:37 	15:05",
		"5 	6 	04:30 	19:39 	15:09",
		"5 	7 	04:28 	19:40 	15:12",
		"5 	8 	04:26 	19:42 	15:16",
		"5 	9 	04:25 	19:43 	15:18",
		"5 	10 	04:23 	19:45 	15:22",
		"5 	11 	04:21 	19:47 	15:26",
		"5 	12 	04:20 	19:48 	15:28",
		"5 	13 	04:18 	19:50 	15:32",
		"5 	14 	04:16 	19:52 	15:36",
		"5 	15 	04:15 	19:53 	15:38",
		"5 	16 	04:13 	19:55 	15:42",
		"5 	17 	04:11 	19:56 	15:45",
		"5 	18 	04:10 	19:58 	15:48",
		"5 	19 	04:09 	19:59 	15:50",
		"5 	20 	04:07 	20:01 	15:54",
		"5 	21 	04:06 	20:02 	15:56",
		"5 	22 	04:04 	20:04 	16:00",
		"5 	23 	04:03 	20:05 	16:02",
		"5 	24 	04:02 	20:07 	16:05",
		"5 	25 	04:01 	20:08 	16:07",
		"5 	26 	04:00 	20:09 	16:09",
		"5 	27 	03:58 	20:11 	16:13",
		"5 	28 	03:57 	20:12 	16:15",
		"5 	29 	03:56 	20:13 	16:17",
		"5 	30 	03:55 	20:14 	16:19",
		"5 	31 	03:54 	20:16 	16:22",
		"6 	1 	03:53 	20:17 	16:24",
		"6 	2 	03:52 	20:18 	16:26",
		"6 	3 	03:52 	20:19 	16:27",
		"6 	4 	03:51 	20:20 	16:29",
		"6 	5 	03:50 	20:21 	16:31",
		"6 	6 	03:50 	20:22 	16:32",
		"6 	7 	03:49 	20:23 	16:34",
		"6 	8 	03:48 	20:24 	16:36",
		"6 	9 	03:48 	20:25 	16:37",
		"6 	10 	03:48 	20:26 	16:38",
		"6 	11 	03:47 	20:26 	16:39",
		"6 	12 	03:47 	20:27 	16:40",
		"6 	13 	03:47 	20:28 	16:41",
		"6 	14 	03:46 	20:28 	16:42",
		"6 	15 	03:46 	20:29 	16:43",
		"6 	16 	03:46 	20:30 	16:44",
		"6 	17 	03:46 	20:30 	16:44",
		"6 	18 	03:46 	20:30 	16:44",
		"6 	19 	03:46 	20:31 	16:45",
		"6 	20 	03:46 	20:31 	16:45",
		"6 	21 	03:46 	20:31 	16:45",
		"6 	22 	03:46 	20:31 	16:45",
		"6 	23 	03:47 	20:32 	16:45",
		"6 	24 	03:47 	20:32 	16:45",
		"6 	25 	03:47 	20:32 	16:45",
		"6 	26 	03:48 	20:32 	16:44",
		"6 	27 	03:48 	20:32 	16:44",
		"6 	28 	03:48 	20:32 	16:44",
		"6 	29 	03:49 	20:31 	16:42",
		"6 	30 	03:50 	20:31 	16:41",
		"7 	1 	03:50 	20:31 	16:41",
		"7 	2 	03:51 	20:31 	16:40",
		"7 	3 	03:52 	20:30 	16:38",
		"7 	4 	03:52 	20:30 	16:38",
		"7 	5 	03:53 	20:29 	16:36",
		"7 	6 	03:54 	20:29 	16:35",
		"7 	7 	03:55 	20:28 	16:33",
		"7 	8 	03:56 	20:28 	16:32",
		"7 	9 	03:57 	20:27 	16:30",
		"7 	10 	03:58 	20:26 	16:28",
		"7 	11 	03:59 	20:25 	16:26",
		"7 	12 	04:00 	20:25 	16:25",
		"7 	13 	04:01 	20:24 	16:23",
		"7 	14 	04:02 	20:23 	16:21",
		"7 	15 	04:03 	20:22 	16:19",
		"7 	16 	04:05 	20:21 	16:16",
		"7 	17 	04:06 	20:20 	16:14",
		"7 	18 	04:07 	20:18 	16:11",
		"7 	19 	04:08 	20:17 	16:09",
		"7 	20 	04:10 	20:16 	16:06",
		"7 	21 	04:11 	20:15 	16:04",
		"7 	22 	04:13 	20:14 	16:01",
		"7 	23 	04:14 	20:12 	15:58",
		"7 	24 	04:15 	20:11 	15:56",
		"7 	25 	04:17 	20:09 	15:52",
		"7 	26 	04:18 	20:08 	15:50",
		"7 	27 	04:20 	20:07 	15:47",
		"7 	28 	04:21 	20:05 	15:44",
		"7 	29 	04:23 	20:03 	15:40",
		"7 	30 	04:24 	20:02 	15:38",
		"7 	31 	04:26 	20:00 	15:34",
		"8 	1 	04:27 	19:59 	15:32",
		"8 	2 	04:29 	19:57 	15:28",
		"8 	3 	04:30 	19:55 	15:25",
		"8 	4 	04:32 	19:53 	15:21",
		"8 	5 	04:34 	19:52 	15:18",
		"8 	6 	04:35 	19:50 	15:15",
		"8 	7 	04:37 	19:48 	15:11",
		"8 	8 	04:38 	19:46 	15:08",
		"8 	9 	04:40 	19:44 	15:04",
		"8 	10 	04:42 	19:42 	15:00",
		"8 	11 	04:43 	19:40 	14:57",
		"8 	12 	04:45 	19:38 	14:53",
		"8 	13 	04:47 	19:36 	14:49",
		"8 	14 	04:48 	19:35 	14:47",
		"8 	15 	04:50 	19:32 	14:42",
		"8 	16 	04:52 	19:30 	14:38",
		"8 	17 	04:53 	19:28 	14:35",
		"8 	18 	04:55 	19:26 	14:31",
		"8 	19 	04:56 	19:24 	14:28",
		"8 	20 	04:58 	19:22 	14:24",
		"8 	21 	05:00 	19:20 	14:20",
		"8 	22 	05:01 	19:18 	14:17",
		"8 	23 	05:03 	19:16 	14:13",
		"8 	24 	05:05 	19:13 	14:08",
		"8 	25 	05:07 	19:11 	14:04",
		"8 	26 	05:08 	19:09 	14:01",
		"8 	27 	05:10 	19:07 	13:57",
		"8 	28 	05:11 	19:05 	13:54",
		"8 	29 	05:13 	19:02 	13:49",
		"8 	30 	05:15 	19:00 	13:45",
		"8 	31 	05:16 	18:58 	13:42",
		"9 	1 	05:18 	18:55 	13:37",
		"9 	2 	05:20 	18:53 	13:33",
		"9 	3 	05:21 	18:51 	13:30",
		"9 	4 	05:23 	18:49 	13:26",
		"9 	5 	05:25 	18:46 	13:21",
		"9 	6 	05:26 	18:44 	13:18",
		"9 	7 	05:28 	18:42 	13:14",
		"9 	8 	05:30 	18:39 	13:09",
		"9 	9 	05:31 	18:37 	13:06",
		"9 	10 	05:33 	18:35 	13:02",
		"9 	11 	05:35 	18:32 	12:57",
		"9 	12 	05:36 	18:30 	12:54",
		"9 	13 	05:38 	18:28 	12:50",
		"9 	14 	05:40 	18:25 	12:45",
		"9 	15 	05:41 	18:23 	12:42",
		"9 	16 	05:43 	18:21 	12:38",
		"9 	17 	05:45 	18:18 	12:33",
		"9 	18 	05:46 	18:16 	12:30",
		"9 	19 	05:48 	18:13 	12:25",
		"9 	20 	05:50 	18:11 	12:21",
		"9 	21 	05:51 	18:09 	12:18",
		"9 	22 	05:53 	18:06 	12:13",
		"9 	23 	05:55 	18:04 	12:09",
		"9 	24 	05:56 	18:02 	12:06",
		"9 	25 	05:58 	17:59 	12:01",
		"9 	26 	06:00 	17:57 	11:57",
		"9 	27 	06:01 	17:54 	11:53",
		"9 	28 	06:03 	17:52 	11:49",
		"9 	29 	06:05 	17:50 	11:45",
		"9 	30 	06:06 	17:47 	11:41",
		"10 	1 	06:08 	17:45 	11:37",
		"10 	2 	06:10 	17:43 	11:33",
		"10 	3 	06:12 	17:40 	11:28",
		"10 	4 	06:13 	17:38 	11:25",
		"10 	5 	06:15 	17:36 	11:21",
		"10 	6 	06:17 	17:33 	11:16",
		"10 	7 	06:18 	17:31 	11:13",
		"10 	8 	06:20 	17:29 	11:09",
		"10 	9 	06:22 	17:26 	11:04",
		"10 	10 	06:24 	17:24 	11:00",
		"10 	11 	06:25 	17:22 	10:57",
		"10 	12 	06:27 	17:20 	10:53",
		"10 	13 	06:29 	17:17 	10:48",
		"10 	14 	06:31 	17:15 	10:44",
		"10 	15 	06:32 	17:13 	10:41",
		"10 	16 	06:34 	17:11 	10:37",
		"10 	17 	06:36 	17:08 	10:32",
		"10 	18 	06:38 	17:06 	10:28",
		"10 	19 	06:40 	17:04 	10:24",
		"10 	20 	06:41 	17:02 	10:21",
		"10 	21 	06:43 	17:00 	10:17",
		"10 	22 	06:45 	16:58 	10:13",
		"10 	23 	06:47 	16:55 	10:08",
		"10 	24 	06:49 	16:53 	10:04",
		"10 	25 	06:50 	16:51 	10:01",
		"10 	26 	06:52 	16:49 	09:57",
		"10 	27 	06:54 	16:47 	09:53",
		"10 	28 	06:56 	16:45 	09:49",
		"10 	29 	06:58 	16:43 	09:45",
		"10 	30 	07:00 	16:41 	09:41",
		"10 	31 	07:01 	16:39 	09:38",
		"11 	1 	07:03 	16:37 	09:34",
		"11 	2 	07:05 	16:35 	09:30",
		"11 	3 	07:07 	16:34 	09:27",
		"11 	4 	07:09 	16:32 	09:23",
		"11 	5 	07:11 	16:30 	09:19",
		"11 	6 	07:13 	16:28 	09:15",
		"11 	7 	07:14 	16:26 	09:12",
		"11 	8 	07:16 	16:25 	09:09",
		"11 	9 	07:18 	16:23 	09:05",
		"11 	10 	07:20 	16:21 	09:01",
		"11 	11 	07:22 	16:20 	08:58",
		"11 	12 	07:24 	16:18 	08:54",
		"11 	13 	07:25 	16:17 	08:52",
		"11 	14 	07:27 	16:15 	08:48",
		"11 	15 	07:29 	16:14 	08:45",
		"11 	16 	07:31 	16:12 	08:41",
		"11 	17 	07:33 	16:11 	08:38",
		"11 	18 	07:34 	16:09 	08:35",
		"11 	19 	07:36 	16:08 	08:32",
		"11 	20 	07:38 	16:07 	08:29",
		"11 	21 	07:40 	16:06 	08:26",
		"11 	22 	07:41 	16:04 	08:23",
		"11 	23 	07:43 	16:03 	08:20",
		"11 	24 	07:45 	16:02 	08:17",
		"11 	25 	07:46 	16:01 	08:15",
		"11 	26 	07:48 	16:00 	08:12",
		"11 	27 	07:49 	15:59 	08:10",
		"11 	28 	07:51 	15:58 	08:07",
		"11 	29 	07:53 	15:57 	08:04",
		"11 	30 	07:54 	15:56 	08:02",
		"12 	1 	07:56 	15:55 	07:59",
		"12 	2 	07:57 	15:55 	07:58",
		"12 	3 	07:59 	15:54 	07:55",
		"12 	4 	08:00 	15:54 	07:54",
		"12 	5 	08:01 	15:53 	07:52",
		"12 	6 	08:03 	15:52 	07:49",
		"12 	7 	08:04 	15:52 	07:48",
		"12 	8 	08:05 	15:52 	07:47",
		"12 	9 	08:06 	15:51 	07:45",
		"12 	10 	08:08 	15:51 	07:43",
		"12 	11 	08:09 	15:51 	07:42",
		"12 	12 	08:10 	15:51 	07:41",
		"12 	13 	08:11 	15:51 	07:40",
		"12 	14 	08:12 	15:51 	07:39",
		"12 	15 	08:13 	15:51 	07:38",
		"12 	16 	08:14 	15:51 	07:37",
		"12 	17 	08:14 	15:51 	07:37",
		"12 	18 	08:15 	15:51 	07:36",
		"12 	19 	08:16 	15:52 	07:36",
		"12 	20 	08:17 	15:52 	07:35",
		"12 	21 	08:17 	15:52 	07:35",
		"12 	22 	08:18 	15:53 	07:35",
		"12 	23 	08:18 	15:53 	07:35",
		"12 	24 	08:19 	15:54 	07:35",
		"12 	25 	08:19 	15:55 	07:36",
		"12 	26 	08:19 	15:55 	07:36",
		"12 	27 	08:20 	15:56 	07:36",
		"12 	28 	08:20 	15:57 	07:37",
		"12 	29 	08:20 	15:58 	07:38",
		"12 	30 	08:20 	15:59 	07:39",
		"12 	31 	08:20 	16:00 	07:40"};




// For a given hour, is it daylight
// Month	Day	Sunrise	Sunset	Daylength
// Returns TRUE if it is daylight, FALSE if not daylight or -1 on error
int isdaylight()
{
        struct tm *current;
        time_t now;
	int msm=0;										// mins since midnight
	int rise_hh,rise_mm;
	int set_hh,set_mm;
	int rise_msm, set_msm;
	//int hh;
	//int mm;
	int i;
	int daynow=FALSE;

	int m,d;										// month day
	//char sr[32];										// sunrise
	//char ss[32];										// sunset
	char dl[32];										// daylength

        time(&now);
        current = localtime(&now);
	msm= (current->tm_hour*60) + current->tm_min;						// how many mins is it since midnight

	for (i=0;i<366;i++)
	{
		//sscanf(risesettable[i],"%d %d %s %s %s",&m,&d,sr,ss,dl);
		sscanf(risesettable[i],"%d %d %d:%d %d:%d %s",&m,&d,&rise_hh,&rise_mm,&set_hh,&set_mm,dl);
		if ( (current->tm_mon+1==m) & (current->tm_mday==d) )				// we found the entry in the table
		{
			rise_msm=(rise_hh*60) + rise_mm;					// rise time in mins since midnight
			set_msm=(set_hh*60) + set_mm;						// set time in mins since midnight
			daynow=FALSE;
			if ( (msm > rise_msm) & (msm <set_msm) )				// are we inside the daylight period ?
				daynow=TRUE;
			//printf("rise_hh=%d  rise_mm=%d   set_hh=%d   set_mm=%d    msm=%d   rise_msm=%d  set_msm=%d\n",rise_hh,rise_mm,set_hh,set_mm,msm,rise_msm,set_msm);	
			//if (daynow==TRUE)
				//printf(" daylight\n");
			//else	printf(" night\n");
			//printf("Month=%d\t day=%d\t sunrise=%s\t sunset=%s\t daylength=%s\n",m,d,sr,ss,dl);
			return(daynow);
		}
	}
	return(-1);										// we should never get here
}


