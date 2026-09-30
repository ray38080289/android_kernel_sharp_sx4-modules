#include "sitronix_ts.h"
#include "sitronix_ts_short_test_cfg.h"
#include <linux/fs.h>

#ifdef CONFIG_TOUCHSCREEN_SITRONIX_TS_RAWDATA
#define FIH_GKI_MODULE_DRIVER 0
#else
#define FIH_GKI_MODULE_DRIVER 1
#endif

#define STX_CRITERION_TEST
#define STX_FLATNESS_TEST

#ifdef ST_TEST_RAW

//#define ST_RAW_LOG_PATH "/sdcard/ST_RAW_LOG.txt"
#define ST_RAW_LOG_PATH "/data/vendor/misc/touch/ST_RAW_LOG.txt"
#define MAX_RX_COUNT 32
#define MAX_TX_COUNT 16
#define MAX_KEY_COUNT 4
#define MAX_SENSOR_COUNT (MAX_RX_COUNT + MAX_TX_COUNT + MAX_KEY_COUNT)
#define MAX_RAW_LIMIT 4300
#define MIN_RAW_LIMIT 1100

#define MAX_RAW_LIMIT_ST14348B 3850
#define MIN_RAW_LIMIT_ST14348B 910

#define ST_RAWTEST_LOGFILE
//#define ST_SHORT_TEST_IS_1T2R		1
#define ST_SHORT_TEST_CFG_OFFSET 0xEFC0

extern void sitronix_touch_tpfwver_read(char *fw_ver);

extern int fih_st_tp_chip_type;

//extern struct pm_qos_request i2c_req;

#endif /* ST_TEST_RAW */

#ifdef ST_TEST_RAW

#ifdef STX_CRITERION_TEST

#define CRITERION_RATIO_MAX		125
#define CRITERION_RATIO_MIN		70

int g_rawdata_golden[MAX_RX_COUNT][MAX_TX_COUNT] = {{0,0,0,0,0,0,0,0,0,0,0,0,0},\
{1893,2236,2261,2246,2210,1862,919,1843,2177,2210,2215,2179,1894},\
{2683,3033,2591,2537,2477,2407,2298,2321,2308,2281,2253,2215,2131},\
{2685,2590,2557,2876,2477,2393,2383,2312,2269,2241,2215,2183,2168},\
{2775,2652,2601,2597,2926,2489,2491,2391,2343,2315,2289,2258,2223},\
{2719,2596,2548,2497,2471,2460,2848,2379,2302,2273,2248,2219,2208},\
{2794,2669,2619,2567,2526,2495,2535,2839,2418,2344,2317,2286,2251},\
{2774,2647,2600,2549,2509,2480,2490,2452,2777,2361,2308,2277,2240},\
{2800,2673,2625,2574,2533,2504,2511,2429,2414,2778,2362,2304,2265},\
{2828,2703,2656,2605,2565,2535,2534,2458,2417,2422,2791,2367,2299},\
{2820,2695,2648,2599,2561,2532,2534,2459,2417,2394,2399,2763,2350},\
{2844,2712,2666,2617,2579,2551,2556,2478,2436,2412,2389,2409,2771},\
{2872,2738,2691,2643,2606,2580,2584,2507,2467,2442,2420,2393,2361},\
{2843,2705,2662,2616,2582,2555,2558,2487,2449,2426,2404,2379,2368},\
{2927,2751,2705,2655,2619,2594,2597,2523,2485,2462,2440,2414,2374},\
{2285,2403,2426,2430,2444,2448,2424,2485,2520,2548,2578,2610,2740},\
{2427,2520,2541,2546,2559,2580,2618,2610,2625,2650,2678,2709,2829},\
{2447,2536,2556,2559,2572,2594,2638,2621,2633,2656,2683,2713,2832},\
{2476,2564,2583,2586,2597,2618,2661,2643,2652,2672,2696,2722,2836},\
{2523,2613,2632,2634,2645,2666,2707,2688,2696,2716,2738,2764,2875},\
{2547,2636,2655,2657,2667,2686,2726,2706,2712,2731,2751,2774,2878},\
{2569,2658,2675,2676,2685,2703,2740,2720,2725,2741,2758,2779,2879},\
{2626,2718,2734,2735,2744,2761,2796,2776,2780,2794,2811,2830,2926},\
{2645,2736,2751,2753,2762,2777,2811,2791,2794,2808,2822,2838,2926},\
{2678,2771,2785,2786,2795,2809,2838,2817,2817,2827,2838,2851,2933},\
{2742,2839,2854,2854,2863,2876,2904,2882,2880,2887,2895,2905,2976},\
{2761,2869,2887,2886,2894,2906,2930,2908,2905,2911,2915,2920,2982},\
{2476,2850,2878,2879,2890,2901,2924,2904,2901,2905,2905,2909,3069}\
};


#define CRITERION_RATIO_MAX_ST14348B		130
#define CRITERION_RATIO_MIN_ST14348B		70

int g_rawdata_golden_ST14348B[MAX_RX_COUNT][MAX_TX_COUNT] = {{0,0,0,0,0,0,0,0,0,0,0,0,0},\
{1442,2161,2219,2204,2178,1817,1000,1803,2151,2185,2188,2155,1789},\
{2557,2540,2481,2427,2388,2326,2234,2252,2236,2210,2182,2155,2094},\
{2598,2487,2434,2392,2341,2309,2314,2239,2199,2174,2148,2128,2149},\
{2683,2557,2503,2451,2420,2380,2396,2310,2266,2242,2218,2200,2206},\
{2628,2505,2453,2401,2361,2332,2359,2263,2220,2198,2174,2159,2189},\
{2697,2571,2516,2464,2423,2391,2409,2334,2281,2258,2236,2221,2230},\
{2679,2552,2500,2449,2409,2380,2397,2314,2285,2253,2231,2217,2225},\
{2707,2580,2526,2476,2435,2406,2423,2340,2300,2289,2257,2242,2252},\
{2728,2603,2552,2502,2464,2434,2443,2371,2332,2312,2301,2277,2284},\
{2720,2596,2545,2500,2463,2435,2446,2372,2338,2317,2298,2296,2291},\
{2739,2609,2559,2511,2475,2447,2462,2386,2349,2330,2310,2299,2319},\
{2767,2636,2585,2539,2502,2478,2492,2418,2382,2364,2347,2334,2344},\
{2739,2607,2560,2517,2484,2458,2470,2402,2369,2352,2334,2324,2359},\
{2815,2652,2602,2557,2524,2499,2511,2442,2408,2390,2372,2362,2372},\
{2237,2320,2333,2336,2350,2352,2338,2392,2429,2459,2489,2532,2666},\
{2389,2431,2440,2442,2456,2473,2519,2508,2524,2554,2583,2625,2755},\
{2400,2435,2444,2446,2459,2477,2528,2509,2524,2551,2579,2619,2748},\
{2439,2471,2478,2479,2490,2507,2557,2534,2546,2570,2595,2632,2755},\
{2485,2517,2523,2524,2535,2551,2599,2578,2588,2613,2637,2672,2793},\
{2507,2538,2544,2544,2554,2568,2616,2593,2602,2624,2645,2678,2793},\
{2533,2561,2566,2566,2575,2587,2631,2607,2616,2635,2654,2684,2792},\
{2598,2625,2629,2628,2636,2647,2690,2668,2673,2690,2707,2733,2832},\
{2617,2644,2649,2648,2656,2665,2706,2682,2687,2704,2718,2743,2833},\
{2649,2676,2681,2680,2686,2695,2732,2707,2709,2722,2731,2752,2832},\
{2715,2745,2752,2750,2757,2764,2798,2772,2771,2782,2790,2807,2876},\
{2728,2772,2784,2784,2790,2794,2825,2797,2796,2806,2809,2823,2886},\
{2369,2768,2794,2795,2804,2808,2833,2803,2799,2804,2802,2804,2624}\
};

#endif

#ifdef STX_FLATNESS_TEST
int gDiff_left_golden[MAX_RX_COUNT][MAX_TX_COUNT] = {\
{1100,325,315,336,748,4000,4000,734,333,305,336,1100},\
{750,942,354,360,370,409,323,313,327,328,338,384},\
{395,333,719,799,384,310,371,343,328,326,332,315},\
{423,351,304,729,837,302,400,348,328,326,331,335},\
{423,348,351,326,311,788,869,377,329,325,329,311},\
{425,350,352,341,331,340,704,821,374,327,331,335},\
{427,347,351,340,329,310,338,725,816,353,331,337},\
{427,348,351,341,329,307,382,315,764,816,358,339},\
{425,347,351,340,330,301,376,341,305,769,824,368},\
{425,347,349,338,329,302,375,342,323,305,764,813},\
{432,346,349,338,328,305,378,342,324,323,320,762},\
{434,347,348,337,326,304,377,340,325,322,327,332},\
{438,343,346,334,327,303,371,338,323,322,325,311},\
{476,346,350,336,325,303,374,338,323,322,326,340},\
{418,323,304,314,304,324,361,335,328,330,332,430},\
{393,321,305,313,321,338,308,315,325,328,331,420},\
{389,320,303,313,322,344,317,312,323,327,330,419},\
{388,319,303,311,321,343,318,309,320,324,326,414},\
{390,319,302,311,321,341,319,308,320,322,326,411},\
{389,319,302,310,319,340,320,306,319,320,323,404},\
{389,317,301,309,318,337,320,305,316,317,321,400},\
{392,316,301,309,317,335,320,304,314,317,319,396},\
{391,315,302,309,315,334,320,303,314,314,316,388},\
{393,314,301,309,314,329,321,300,310,311,313,382},\
{397,315,300,309,313,328,322,302,307,308,310,371},\
{408,318,301,308,312,324,322,303,306,304,305,362},\
{1100,328,301,311,311,323,320,303,304,300,304,1100}\
};

int gDiff_up_golden[MAX_RX_COUNT][MAX_TX_COUNT] = {\
{1190,1197,730,691,667,945,4000,878,431,371,338,336,1100},\
{302,843,334,739,300,314,385,309,339,340,338,332,337},\
{390,362,344,679,949,396,408,379,374,374,374,375,355},\
{356,356,353,400,955,329,757,312,341,342,341,339,315},\
{375,373,371,370,355,335,713,860,416,371,369,367,343},\
{320,322,319,318,317,315,345,787,759,317,309,309,311},\
{326,326,325,325,324,324,321,323,763,817,354,327,325},\
{328,330,331,331,332,331,323,329,303,756,829,363,334},\
{308,308,308,306,304,303,300,301,300,328,792,796,351},\
{324,317,318,318,318,319,322,319,319,318,310,754,810},\
{328,326,325,326,327,329,328,329,331,330,331,316,810},\
{329,333,329,327,324,325,326,320,318,316,316,314,307},\
{384,346,343,339,337,339,339,336,336,336,336,335,306},\
{1042,748,679,625,475,446,473,338,335,386,438,496,766},\
{442,417,415,416,415,432,494,425,405,402,400,399,389},\
{320,316,315,313,313,314,320,311,308,306,305,304,303},\
{329,328,327,327,325,324,323,322,319,316,313,309,304},\
{347,349,349,348,348,348,346,345,344,344,342,342,339},\
{324,323,323,323,322,320,319,318,316,315,313,310,303},\
{322,322,320,319,318,317,314,314,313,310,307,305,301},\
{357,360,359,359,359,358,356,356,355,353,353,351,347},\
{319,318,317,318,318,316,315,315,314,314,311,308,300},\
{333,335,334,333,333,332,327,326,323,319,316,313,307},\
{364,368,369,368,368,367,366,365,363,360,357,354,343},\
{319,330,333,332,331,330,326,326,325,324,320,315,306},\
{1100,319,309,307,304,305,306,304,304,306,310,311,1100}\
};


int gDiff_left_golden_ST14348B[MAX_RX_COUNT][MAX_TX_COUNT] = {\
{1169,508,465,476,811,3817,3803,798,484,453,483,816},\
{467,409,404,390,411,442,368,365,376,378,377,511},\
{562,403,392,401,383,355,424,391,375,376,370,471},\
{576,404,402,380,391,366,436,394,374,374,367,455},\
{573,402,402,390,380,378,447,392,372,374,366,480},\
{576,405,402,391,381,368,425,403,372,372,365,459},\
{577,401,402,390,379,367,433,379,382,372,364,457},\
{577,404,401,390,380,367,433,389,361,382,365,459},\
{575,401,400,388,380,359,422,389,370,360,374,457},\
{574,400,396,387,378,361,423,385,371,369,352,455},\
{579,400,398,387,378,365,426,387,369,370,361,470},\
{582,400,396,387,375,364,423,386,368,367,363,460},\
{581,397,393,383,376,362,418,382,368,367,361,485},\
{614,399,396,383,375,363,420,384,368,368,360,460},\
{533,362,354,364,351,364,404,387,380,380,393,583},\
{491,359,353,364,367,396,361,366,379,379,393,580},\
{485,359,352,363,368,401,369,365,377,377,390,580},\
{482,357,351,361,367,400,372,362,374,375,387,573},\
{482,356,351,361,366,398,371,360,374,374,385,571},\
{481,356,350,360,364,397,373,359,372,371,383,565},\
{479,354,350,359,362,394,373,359,369,369,380,559},\
{478,354,352,358,362,392,372,355,367,367,376,549},\
{477,355,351,358,359,391,374,355,367,364,375,540},\
{477,355,351,356,359,387,375,352,362,360,371,530},\
{481,356,351,357,357,384,376,351,361,358,367,519},\
{494,361,350,357,354,381,378,351,360,354,364,513},\
{1099,475,452,459,453,476,481,453,455,452,452,930}\
};


int gDiff_up_golden_ST14348B[MAX_RX_COUNT][MAX_TX_COUNT] = {\
{1865,779,1012,973,960,1260,4584,899,535,475,455,450,1055},\
{491,403,397,385,396,368,430,362,388,386,384,377,505},\
{534,420,419,409,429,421,432,421,418,418,420,422,507},\
{505,402,400,399,409,398,386,397,396,394,394,392,467},\
{519,416,413,412,412,410,400,421,410,410,412,412,491},\
{468,369,366,365,364,362,362,370,354,356,355,354,456},\
{478,378,376,377,377,376,376,376,366,387,376,375,477},\
{471,374,376,377,379,378,370,381,382,372,395,385,482},\
{459,358,357,352,351,351,352,351,356,355,354,368,457},\
{469,364,364,362,362,362,367,364,362,363,362,353,478},\
{479,376,376,378,378,381,379,382,383,384,387,385,475},\
{479,378,375,372,368,370,372,367,363,362,363,361,464},\
{527,394,392,390,390,391,391,390,388,388,388,389,463},\
{1128,781,720,671,524,497,524,400,372,419,467,620,844},\
{602,460,457,456,456,472,531,466,445,444,443,443,540},\
{461,354,354,354,353,354,359,351,350,352,354,356,457},\
{488,386,384,383,381,380,378,375,372,369,366,363,457},\
{497,396,396,396,395,394,393,394,392,392,392,389,488},\
{472,371,370,370,369,367,366,365,363,361,358,357,451},\
{476,374,372,372,371,368,365,365,364,361,359,355,451},\
{515,414,414,412,411,411,409,410,407,405,403,400,490},\
{469,369,369,370,370,368,366,364,364,364,362,360,452},\
{482,381,382,382,380,379,376,375,372,368,363,359,451},\
{516,420,421,421,421,419,416,416,412,410,408,405,493},\
{463,377,382,383,383,380,377,374,375,374,370,366,460},\
{1109,454,460,461,464,464,458,456,453,452,457,469,1012}\
};

#endif

static st_int st_drv_Get_2D_Length(st_int tMode[])
{
	if (tMode[0] == 0)
		return tMode[1];
	else
		return tMode[2];
}
static st_int st_drv_Get_2D_Count(st_int tMode[])
{
	if (tMode[0] == 0)
		return tMode[2];
	else
		return tMode[1];
}

#ifdef ST_RAWTEST_LOGFILE
static st_int st_drv_Get_2D_RAW_ST14348B(st_int tMode[], st_int rawJ[], st_int gsMode, st_u8 *rtbuf, struct file *filp,loff_t *ppos)
#else
static st_int st_drv_Get_2D_RAW_ST14348B(st_int tMode[], st_int rawJ[], st_int gsMode, st_u8 *rtbuf)
#endif /* ST_RAWTEST_LOGFILE */
{
	st_int count = st_drv_Get_2D_Count(tMode);
	st_int length = st_drv_Get_2D_Length(tMode);
	st_int maxTimes = 60;
	st_int dataCount = 0;
	st_int times = maxTimes;
	st_int readLength = 8 + 2 * length;
	st_u8 raw[0x40];
	st_int i = 0;
	st_int index;
	st_int keyAddCount = (tMode[3] > 0) ? 1 : 0;
	short rawI;
#ifdef STX_CRITERION_TEST
	st_int j = 0;
	short (*criterion_result)[MAX_TX_COUNT];
	int criterion_ratio = 0;
#endif
	st_int errorCount = 0;
	st_u8 isFillData[MAX_SENSOR_COUNT];
#ifdef ST_RAWTEST_LOGFILE
	char data1[80];
#endif

#ifdef STX_FLATNESS_TEST
	short (*rawdata_map)[MAX_TX_COUNT];
	int Dvalue;

	rawdata_map = kmalloc(MAX_RX_COUNT * MAX_TX_COUNT * sizeof(short), GFP_KERNEL);
	if (!rawdata_map) {
		STX_DEBUG("rawdata_map kmalloc fail");
		return -1;
	}
	memset(rawdata_map, 0, MAX_RX_COUNT * MAX_TX_COUNT);
#endif

#ifdef STX_CRITERION_TEST
	criterion_result = kmalloc(MAX_RX_COUNT * MAX_TX_COUNT * sizeof(short), GFP_KERNEL);
	if (!criterion_result) {
		STX_DEBUG("criterion_result kmalloc fail");
		kfree(rawdata_map);
		return -1;
	}
	memset(criterion_result, 0, MAX_RX_COUNT * MAX_TX_COUNT);
#endif

	memset(isFillData, 0, MAX_SENSOR_COUNT);
	memset(raw, 0, 0x40);

//hfst add for jump 2 frame
	times = maxTimes*2;
	while (dataCount < (2*(count + keyAddCount+1)) && times-- > 0)
	{
		stx_i2c_read_bytes(0x40, raw, readLength);
		if (raw[0] == 6)
		{
			index = raw[2];
			dataCount++;
		}
		else if (raw[0] == 7)
		{
			dataCount++;
		}
	}

	dataCount = 0;
	times = maxTimes;
	memset(isFillData, 0, MAX_SENSOR_COUNT);
	memset(raw, 0, 0x40);
//hfst modify end

	/* STX_DEBUG("isFill 0 : %d",isFillData[0]); */
	while (dataCount != (count + keyAddCount) && times-- > 0)
	{
		stx_i2c_read_bytes(0x40, raw, readLength);
		/* STX_DEBUG("%X %X %X data:%d key:%d",raw[0],raw[1],raw[2],dataCount,tMode[3]); */
		if (raw[0] == 6)
		{
			index = raw[2];
			/* STX_DEBUG("isFill %d : %d %d , %d",index,isFillData[index],dataCount,count+keyAddCount); */
			if (isFillData[index] == 0)
			{
				/* STX_DEBUG("index %d",index); */
				isFillData[index] = 1;

				/* index = index*length; */
				if (index != 0)
				{
					dataCount++;
#ifdef ST_RAWTEST_LOGFILE
					snprintf(data1, 80, "index%d:",index);
#if !FIH_GKI_MODULE_DRIVER
					kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
					for (i = 0; i < length; i++)
					{
						rawI = (short)((raw[4 + 2 * i]) * 0x100 + raw[5 + 2 * i]);
#ifdef STX_FLATNESS_TEST
						rawdata_map[index][i] = rawI;
#endif

#ifdef STX_CRITERION_TEST
						if(g_rawdata_golden_ST14348B[index][i] != 0)
							criterion_result[index][i] = (rawI * 100)/g_rawdata_golden_ST14348B[index][i];
#endif
#ifdef ST_RAWTEST_LOGFILE
						snprintf(data1, 80, "%d,",rawI);
#if !FIH_GKI_MODULE_DRIVER
						kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
						/* STX_DEBUG("Sensor RAW %d,%d = %d",index,i,rawI); */
						if((index == 1) && ((i == 0) || (i == 5) || (i == 6) || (i == 7)))
						{
#ifdef STX_CRITERION_TEST
							criterion_result[index][i] = 100;
#endif
							STX_DEBUG("IGNORED TEST THESE NODES index%d,i%d",index,i);
						}else{
						if (rawI > MAX_RAW_LIMIT_ST14348B || rawI < MIN_RAW_LIMIT_ST14348B)
						{
							STX_ERROR("Error: Sensor RAW %d,%d = %d out of limity (%d,%d)", index, i, rawI, MIN_RAW_LIMIT_ST14348B, MAX_RAW_LIMIT_ST14348B);
#ifdef ST_RAWTEST_LOGFILE
							snprintf(data1, 80, "Error: Sensor RAW %d,%d = %d out of limity (%d,%d)\n", index, i, rawI, MIN_RAW_LIMIT_ST14348B, MAX_RAW_LIMIT_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
							kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
							rtbuf[index * length + i] = 1;
							errorCount++;
						}
						/* rawI[index+i] = raw[4+2*i]*0x100 + raw[5+2*i]; */
						}
					}
#ifdef ST_RAWTEST_LOGFILE
						snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
						kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
				}
			}
		}
		else if (raw[0] == 7)
		{
			/* key */
			STX_INFO("key");
			if (isFillData[count] == 0)
			{
				isFillData[count] = 1;
				dataCount++;
				for (i = 0; i < tMode[3]; i++)
				{
					rawI = (short)((raw[4 + 2 * i]) * 0x100 + raw[5 + 2 * i]);
					/* STX_DEBUG("Key RAW %d = %d",i,rawI); */
					if (rawI > MAX_RAW_LIMIT_ST14348B || rawI < MIN_RAW_LIMIT_ST14348B)
					{
						STX_ERROR("Error: Key RAW %d = %d out of limity (%d,%d)", i, rawI, MIN_RAW_LIMIT_ST14348B, MAX_RAW_LIMIT_ST14348B);
#ifdef ST_RAWTEST_LOGFILE
						snprintf(data1, 80, "Error: Key RAW %d = %d out of limity (%d,%d)\n", i, rawI, MIN_RAW_LIMIT_ST14348B, MAX_RAW_LIMIT_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
						kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
						rtbuf[count * length + i] = 1;
						errorCount++;
					}
					/* rawI[count*length+i] = raw[4+2*i]*0x100 + raw[5+2*i]; */
				}
			}
		}
	}
	if (times <= 0)
	{
		STX_ERROR("Get 2D RAW fail!");
#ifdef STX_FLATNESS_TEST
		kfree(rawdata_map);
#endif
#ifdef STX_CRITERION_TEST
		kfree(criterion_result);
#endif
		return -1;
	}

#ifdef STX_CRITERION_TEST

	for(i = 1;i < (count + 1);i++)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "Criterion_Percent%d:",i);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("Criterion_Percent%d:",i);
		for(j = 0;j < length;j++)
		{
			criterion_ratio = criterion_result[i][j];
#ifdef ST_RAWTEST_LOGFILE
			snprintf(data1, 80, "%d,",criterion_ratio);
#if !FIH_GKI_MODULE_DRIVER
			kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			printk("%d,",criterion_ratio);
			if((criterion_ratio > CRITERION_RATIO_MAX_ST14348B)||(criterion_ratio < CRITERION_RATIO_MIN_ST14348B))
			{
				errorCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data1, 80, "Error: Sensor criterion_ratio %d = %d out of limity (%d,%d)\n", j, criterion_ratio, CRITERION_RATIO_MIN_ST14348B, CRITERION_RATIO_MAX_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
				kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
				printk("\nError: Sensor criterion_ratio %d = %d out of limity (%d,%d)\n", j, criterion_ratio, CRITERION_RATIO_MIN_ST14348B, CRITERION_RATIO_MAX_ST14348B);
			}
		}
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("\n");
	}
#endif


#ifdef STX_FLATNESS_TEST

#ifdef ST_RAWTEST_LOGFILE
	snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
	kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
	for(i = 1;i < (count + 1 - 1);i++)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "Flatness_Test_UP %d:",i);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("Flatness_Test_UP %d:",i);
		for(j = 0;j < length;j++)
		{
			if(rawdata_map[i][j] > rawdata_map[i + 1][j])
				Dvalue = rawdata_map[i][j] - rawdata_map[i + 1][j];
			else
				Dvalue = rawdata_map[i + 1][j] - rawdata_map[i][j];
#ifdef ST_RAWTEST_LOGFILE
			snprintf(data1, 80, "%d,",Dvalue);
#if !FIH_GKI_MODULE_DRIVER
			kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			printk("%d,",Dvalue);
			if(Dvalue > gDiff_up_golden_ST14348B[i - 1][j])
			{
				errorCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data1, 80, "Error: Sensor Flatness %d = %d out of limity (%d)\n", j, Dvalue, gDiff_up_golden_ST14348B[i - 1][j]);
#if !FIH_GKI_MODULE_DRIVER
				kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			}
		}
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("\n");
	}

#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif

	for(i = 1;i < (count + 1);i++)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "Flatness_Test_LEFT %d:",i);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("Flatness_Test_LEFT %d:",i);
		for(j = 0;j < (length - 1);j++)
		{
			if(rawdata_map[i][j] > rawdata_map[i][j + 1])
				Dvalue = rawdata_map[i][j] - rawdata_map[i][j + 1];
			else
				Dvalue = rawdata_map[i][j + 1] - rawdata_map[i][j];
#ifdef ST_RAWTEST_LOGFILE
			snprintf(data1, 80, "%d,",Dvalue);
#if !FIH_GKI_MODULE_DRIVER
			kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			printk("%d,",Dvalue);
			
			if(Dvalue > gDiff_left_golden_ST14348B[i - 1][j])
			{
				errorCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data1, 80, "Error: Sensor Flatness %d = %d out of limity (%d)\n", j, Dvalue, gDiff_left_golden_ST14348B[i - 1][j]);
#if !FIH_GKI_MODULE_DRIVER
				kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			}
		}
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("\n");
	}
#endif

#ifdef STX_FLATNESS_TEST
		kfree(rawdata_map);
#endif
#ifdef STX_CRITERION_TEST
		kfree(criterion_result);
#endif

	return errorCount;
}


#ifdef ST_RAWTEST_LOGFILE
static st_int st_drv_Get_2D_RAW(st_int tMode[], st_int rawJ[], st_int gsMode, st_u8 *rtbuf, struct file *filp,loff_t *ppos)
#else
static st_int st_drv_Get_2D_RAW(st_int tMode[], st_int rawJ[], st_int gsMode, st_u8 *rtbuf)
#endif /* ST_RAWTEST_LOGFILE */
{
	st_int count = st_drv_Get_2D_Count(tMode);
	st_int length = st_drv_Get_2D_Length(tMode);
	st_int maxTimes = 60;
	st_int dataCount = 0;
	st_int times = maxTimes;
	st_int readLength = 8 + 2 * length;
	st_u8 raw[0x40];
	st_int i = 0;
	st_int index;
	st_int keyAddCount = (tMode[3] > 0) ? 1 : 0;
	short rawI;
#ifdef STX_CRITERION_TEST
	st_int j = 0;
	short (*criterion_result)[MAX_TX_COUNT];
	int criterion_ratio = 0;
#endif
	st_int errorCount = 0;
	st_u8 isFillData[MAX_SENSOR_COUNT];
#ifdef ST_RAWTEST_LOGFILE
	char data1[80];
#endif

#ifdef STX_FLATNESS_TEST
	short (*rawdata_map)[MAX_TX_COUNT];
	int Dvalue;

	rawdata_map = kmalloc(MAX_RX_COUNT * MAX_TX_COUNT * sizeof(short), GFP_KERNEL);
	if (!rawdata_map) {
		STX_DEBUG("rawdata_map kmalloc fail");
		return -1;
	}
	memset(rawdata_map, 0, MAX_RX_COUNT * MAX_TX_COUNT);
#endif

#ifdef STX_CRITERION_TEST
	criterion_result = kmalloc(MAX_RX_COUNT * MAX_TX_COUNT * sizeof(short), GFP_KERNEL);
	if (!criterion_result) {
		STX_DEBUG("criterion_result kmalloc fail");
		kfree(rawdata_map);
		return -1;
	}
	memset(criterion_result, 0, MAX_RX_COUNT * MAX_TX_COUNT);
#endif

	memset(isFillData, 0, MAX_SENSOR_COUNT);
	memset(raw, 0, 0x40);

//hfst add for jump 2 frame
	times = maxTimes*2;
	while (dataCount < (2*(count + keyAddCount+1)) && times-- > 0)
	{
		stx_i2c_read_bytes(0x40, raw, readLength);
		if (raw[0] == 6)
		{
			index = raw[2];
//			if (isFillData[index] == 0)
//			{
//				isFillData[index] = 1;
				dataCount++;
//			}
		}
		else if (raw[0] == 7)
		{
//			if (isFillData[count] == 0)
//			{
//				isFillData[count] = 1;
				dataCount++;
//			}
		}
	}

	dataCount = 0;
	times = maxTimes;
	memset(isFillData, 0, MAX_SENSOR_COUNT);
	memset(raw, 0, 0x40);
//hfst modify end

	/* STX_DEBUG("isFill 0 : %d",isFillData[0]); */
	while (dataCount != (count + keyAddCount) && times-- > 0)
	{
		stx_i2c_read_bytes(0x40, raw, readLength);
		/* STX_DEBUG("%X %X %X data:%d key:%d",raw[0],raw[1],raw[2],dataCount,tMode[3]); */
		if (raw[0] == 6)
		{
			index = raw[2];
			/* STX_DEBUG("isFill %d : %d %d , %d",index,isFillData[index],dataCount,count+keyAddCount); */
			if (isFillData[index] == 0)
			{
				/* STX_DEBUG("index %d",index); */
				isFillData[index] = 1;

				/* index = index*length; */
				if (index != 0)
				{
					dataCount++;
#ifdef ST_RAWTEST_LOGFILE
					snprintf(data1, 80, "index%d:",index);
#if !FIH_GKI_MODULE_DRIVER
					kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
					for (i = 0; i < length; i++)
					{
						rawI = (short)((raw[4 + 2 * i]) * 0x100 + raw[5 + 2 * i]);
#ifdef STX_FLATNESS_TEST
						rawdata_map[index][i] = rawI;
#endif

#ifdef STX_CRITERION_TEST
						if(g_rawdata_golden[index][i] != 0)
							criterion_result[index][i] = (rawI * 100)/g_rawdata_golden[index][i];
#endif
#ifdef ST_RAWTEST_LOGFILE
						snprintf(data1, 80, "%d,",rawI);
#if !FIH_GKI_MODULE_DRIVER
						kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
						/* STX_DEBUG("Sensor RAW %d,%d = %d",index,i,rawI); */
						if((index == 1) && ((i == 0) || (i == 5) || (i == 6) || (i == 7)))
						{
#ifdef STX_CRITERION_TEST
							criterion_result[index][i] = 100;
#endif
							STX_DEBUG("IGNORED TEST THESE NODES index%d,i%d",index,i);
						}else{
						if (rawI > MAX_RAW_LIMIT || rawI < MIN_RAW_LIMIT)
						{
							STX_ERROR("Error: Sensor RAW %d,%d = %d out of limity (%d,%d)", index, i, rawI, MIN_RAW_LIMIT, MAX_RAW_LIMIT);
#ifdef ST_RAWTEST_LOGFILE
							snprintf(data1, 80, "Error: Sensor RAW %d,%d = %d out of limity (%d,%d)\n", index, i, rawI, MIN_RAW_LIMIT, MAX_RAW_LIMIT);
#if !FIH_GKI_MODULE_DRIVER
							kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
							rtbuf[index * length + i] = 1;
							errorCount++;
						}
						/* rawI[index+i] = raw[4+2*i]*0x100 + raw[5+2*i]; */
					}
				}
#ifdef ST_RAWTEST_LOGFILE
						snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
						kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
				}
			}
		}
		else if (raw[0] == 7)
		{
			/* key */
			STX_INFO("key");
			if (isFillData[count] == 0)
			{
				isFillData[count] = 1;
				dataCount++;
				for (i = 0; i < tMode[3]; i++)
				{
					rawI = (short)((raw[4 + 2 * i]) * 0x100 + raw[5 + 2 * i]);
					/* STX_DEBUG("Key RAW %d = %d",i,rawI); */
					if (rawI > MAX_RAW_LIMIT || rawI < MIN_RAW_LIMIT)
					{
						STX_ERROR("Error: Key RAW %d = %d out of limity (%d,%d)", i, rawI, MIN_RAW_LIMIT, MAX_RAW_LIMIT);
#ifdef ST_RAWTEST_LOGFILE
						snprintf(data1, 80, "Error: Key RAW %d = %d out of limity (%d,%d)\n", i, rawI, MIN_RAW_LIMIT, MAX_RAW_LIMIT);
#if !FIH_GKI_MODULE_DRIVER
						kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
						rtbuf[count * length + i] = 1;
						errorCount++;
					}
					/* rawI[count*length+i] = raw[4+2*i]*0x100 + raw[5+2*i]; */
				}
			}
		}
	}
	if (times <= 0)
	{
		STX_ERROR("Get 2D RAW fail!");
#ifdef STX_FLATNESS_TEST
		kfree(rawdata_map);
#endif
#ifdef STX_CRITERION_TEST
		kfree(criterion_result);
#endif
		return -1;
	}

#ifdef STX_CRITERION_TEST

	for(i = 1;i < (count + 1);i++)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "Criterion_Percent%d:",i);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("Criterion_Percent%d:",i);
		for(j = 0;j < length;j++)
		{
			criterion_ratio = criterion_result[i][j];
#ifdef ST_RAWTEST_LOGFILE
			snprintf(data1, 80, "%d,",criterion_ratio);
#if !FIH_GKI_MODULE_DRIVER
			kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			printk("%d,",criterion_ratio);
			if((criterion_ratio > CRITERION_RATIO_MAX)||(criterion_ratio < CRITERION_RATIO_MIN))
			{
				errorCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data1, 80, "Error: Sensor criterion_ratio %d = %d out of limity (%d,%d)\n", j, criterion_ratio, CRITERION_RATIO_MIN, CRITERION_RATIO_MAX);
#if !FIH_GKI_MODULE_DRIVER
				kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
				printk("\nError: Sensor criterion_ratio %d = %d out of limity (%d,%d)\n", j, criterion_ratio, CRITERION_RATIO_MIN, CRITERION_RATIO_MAX);
			}
		}
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("\n");
	}
#endif


#ifdef STX_FLATNESS_TEST

#ifdef ST_RAWTEST_LOGFILE
	snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
	kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
	for(i = 1;i < (count + 1 - 1);i++)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "Flatness_Test_UP %d:",i);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("Flatness_Test_UP %d:",i);
		for(j = 0;j < length;j++)
		{
			if(rawdata_map[i][j] > rawdata_map[i + 1][j])
				Dvalue = rawdata_map[i][j] - rawdata_map[i + 1][j];
			else
				Dvalue = rawdata_map[i + 1][j] - rawdata_map[i][j];
#ifdef ST_RAWTEST_LOGFILE
			snprintf(data1, 80, "%d,",Dvalue);
#if !FIH_GKI_MODULE_DRIVER
			kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			printk("%d,",Dvalue);
			if(Dvalue > gDiff_up_golden[i - 1][j])
			{
				errorCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data1, 80, "Error: Sensor Flatness %d = %d out of limity (%d)\n", j, Dvalue, gDiff_up_golden[i - 1][j]);
#if !FIH_GKI_MODULE_DRIVER
				kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			}
		}
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("\n");
	}

#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif

	for(i = 1;i < (count + 1);i++)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "Flatness_Test_LEFT %d:",i);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("Flatness_Test_LEFT %d:",i);
		for(j = 0;j < (length - 1);j++)
		{
			if(rawdata_map[i][j] > rawdata_map[i][j + 1])
				Dvalue = rawdata_map[i][j] - rawdata_map[i][j + 1];
			else
				Dvalue = rawdata_map[i][j + 1] - rawdata_map[i][j];
#ifdef ST_RAWTEST_LOGFILE
			snprintf(data1, 80, "%d,",Dvalue);
#if !FIH_GKI_MODULE_DRIVER
			kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			printk("%d,",Dvalue);
			
			if(Dvalue > gDiff_left_golden[i - 1][j])
			{
				errorCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data1, 80, "Error: Sensor Flatness %d = %d out of limity (%d)\n", j, Dvalue, gDiff_left_golden[i - 1][j]);
#if !FIH_GKI_MODULE_DRIVER
				kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
			}
		}
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 80, "\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
		printk("\n");
	}
#endif
#ifdef STX_FLATNESS_TEST
		kfree(rawdata_map);
#endif
#ifdef STX_CRITERION_TEST
		kfree(criterion_result);
#endif

	return errorCount;
}

static int Get1DRaw(bool fSelectTXMode, int nRawData[])
{
	unsigned char mutBuf[0xFF];
	int count = 0;
	int maxtime = 0;
	int id;

	mutBuf[0] = 0xF1;
	mutBuf[1] = 0xC0;

	stx_i2c_write_bytes(mutBuf, 2);

	while (count < 5 && maxtime++ < 1000)
	{
		/* msleep(20); */
		if (count < 4)
			stx_i2c_read_bytes(0x40, mutBuf, 4);
		else
			stx_i2c_read_bytes(0x40, mutBuf, 0x50);

		if (((!fSelectTXMode) && (mutBuf[0] == 0x04) && (mutBuf[2] == 0x00) && (mutBuf[3] == 0x00)) || ((fSelectTXMode) && (mutBuf[0] == 0x04) && (mutBuf[2] == 0x02)))
		{
			count++;
			if (count == 5)
			{
				for (id = 0; id < (36); id++)
				{
					if (mutBuf[4 + 2 * id] & 0x80)
						nRawData[id] = ((mutBuf[4 + 2 * id] << 8) + mutBuf[5 + 2 * id]) - 65536;
					else
						nRawData[id] = ((mutBuf[4 + 2 * id] << 8) + mutBuf[5 + 2 * id]);
				}
			}
		}
	}
	if (count >= 5)
		STX_INFO("Get 1d RAW ok %d %d %d %d %d %d %d %d", nRawData[0], nRawData[1], nRawData[2], nRawData[3], nRawData[4], nRawData[5], nRawData[6], nRawData[7]);
	else
		STX_ERROR("Get 1d RAW fail");
	return (count >= 5) ? 0 : -1;
}

#if 0
static void WriteFilterRam(void)
{
	/* FilterRam */
	unsigned char pFilterRam[2] = {0x00, 0x30};
	unsigned char pCMDData[1024] = {0};
	int i;

	for (i = 0; i < 1024; i++)
	{
		if ((i % 2) == 0)
			pCMDData[i] = pFilterRam[0];
		else
			pCMDData[i] = pFilterRam[1];
	}
	stx_cmdio_write(3, 0, pCMDData, 0x9A);
}
#endif

static void WriteTestCFG(void)
{
	unsigned char buf[8];

	stx_cmdio_write(2, ST_SHORT_TEST_CFG_OFFSET, st_short_test_cfg, sizeof(st_short_test_cfg));
	/* #DoPower Down Wake Up */
	buf[0] = 0x2;
	buf[1] = 0x2;
	stx_i2c_write_bytes(buf, 2);

	msleep(150);
	buf[0] = 0x2;
	buf[1] = 0x0;
	stx_i2c_write_bytes(buf, 2);
	msleep(150);
	//WriteFilterRam();
}

#ifdef ST_RAWTEST_LOGFILE
static int RunFindShortStart(bool fSelectTXMode, int xNum, int yNum, unsigned char pMaxPatternCount, struct file *filp,loff_t *ppos)
#else
static int RunFindShortStart(bool fSelectTXMode, int xNum, int yNum, unsigned char pMaxPatternCount)
#endif
{
	int nScanChannel = 0;
	int nMaxShortRecordRaw = 0;
	int nShortMaxRawDefine = 0;
	unsigned char pTX_OSB = 0x03, pRX_OSB = 0x03;
	unsigned char pShortSkipArray[40] = {0};

//	int nMaxRx = xNum = 36;
//	int nMaxTx = yNum = 20;

	bool fMayBeFilterRamError = false;
	int nRet = 1, pMutulRaw[40];
	unsigned char pCMDData[64] = {0};
	unsigned char Pattern[13][5]; /* ={0}; */
	unsigned char pListArray[13][37] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
		{0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
		{0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
		{0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
		{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0}};
	unsigned char pSkipArray[37] = {0};
	unsigned char pTempArray_1[37] = {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1};
	unsigned char pTempArray_2[37] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

	int nSum = 0;
	int i, j;

	int nTxStartRun = 0;
	int nRun;

	//unsigned char fFindShort = false;
	int nRaw = 0;

	int nAddTx = 0;
	bool fFind = false;
	int nCheckStart = 0;
#ifdef ST_RAWTEST_LOGFILE
	char data1[80];
#endif
	/* /////////////////// */
	/* ////////// start work */
	if (fSelectTXMode)
		nScanChannel = yNum;
	else
		nScanChannel = xNum;

//	if (fSelectTXMode)
//		nScanChannel = nMaxTx;
//	else
//		nScanChannel = nMaxRx;

	if (fSelectTXMode == false)
		memcpy(pSkipArray, pTempArray_1, 37);
	else
		memcpy(pSkipArray, pTempArray_2, 37);

	for (i = 1; i < 13; i++)
		for (j = 0; j < 36; j++)
			if (pSkipArray[j] == 1)
				pListArray[i][j] = 1;

	/* memset(pResult,1,40*40); */
	for (i = 0; i < 13; i++)
	{
		nSum = 0;
		for (j = 0; j < 37; j++)
		{
			nSum += ((pListArray[i][j]) << (j % 0x08));
			if ((j + 1) % 8 == 0)
			{
				Pattern[i][j / 8] = nSum;
				nSum = 0;
			}
		}
		Pattern[i][37 / 8] = nSum + 0x20;
	}

	/* /////////// */
	if (fSelectTXMode)
		nTxStartRun = 0;
	for (nRun = nTxStartRun; (nRun < pMaxPatternCount) && (fMayBeFilterRamError == false); nRun++)
	{
		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0x1D;
		pCMDData[5] = 0x02;
		pCMDData[6] = 0x2D;
		pCMDData[7] = 0x2D;
		stx_cmdio_write(4, 0x1D, pCMDData + 6, 2);

		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0x28;
		pCMDData[5] = 0x02;
		pCMDData[6] = 0x24;
		pCMDData[7] = 0x00;
		stx_cmdio_write(4, 0x28, pCMDData + 6, 2);

		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0x11;
		pCMDData[5] = 0x02;
		pCMDData[6] = 0x00;
		pCMDData[7] = 0x00;
		stx_cmdio_write(4, 0x11, pCMDData + 6, 2);

		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0x12;
		pCMDData[5] = 0x02;
		pCMDData[6] = 0x00;
		pCMDData[7] = 0x00;
		stx_cmdio_write(4, 0x12, pCMDData + 6, 2);

		/* Set Addr :0x00D1 STOG[15:1] */
		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0xD1;
		pCMDData[5] = 0x02;
		pCMDData[6] = Pattern[nRun][1];
		pCMDData[7] = Pattern[nRun][0];
		stx_cmdio_write(4, 0xD1, pCMDData + 6, 2);

		/* Set Addr :0x00D2 STOG[31:16] */
		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0xD2;
		pCMDData[5] = 0x02;
		pCMDData[6] = Pattern[nRun][3];
		pCMDData[7] = Pattern[nRun][2];
		stx_cmdio_write(4, 0xD2, pCMDData + 6, 2);

		/* Set Addr :0x00D3 STOG[35:32] , OSB:10 <- 0.8uA */
		pCMDData[0] = 0x01;
		pCMDData[1] = 0x07;
		pCMDData[2] = 0x04;
		pCMDData[3] = 0x00;
		pCMDData[4] = 0xD3;
		pCMDData[5] = 0x02;
		pCMDData[6] = 0x03;
		if (fSelectTXMode)
			pCMDData[7] = ((Pattern[nRun][4] & 0x0F) | (pTX_OSB << 4));
		else
			pCMDData[7] = ((Pattern[nRun][4] & 0x0F) | (pRX_OSB << 4));

		STX_INFO("Pattern0-3 %x %x %x %x,pCMDData[6][7] %x %x", Pattern[nRun][0], Pattern[nRun][1], Pattern[nRun][2], Pattern[nRun][3], pCMDData[6], pCMDData[7]);
		stx_cmdio_write(4, 0xD3, pCMDData + 6, 2);

		if (Get1DRaw(fSelectTXMode, pMutulRaw) < 0)
		{
			STX_ERROR("Get1DRaw(fSelectTXMode,pMutulRaw) < 0");
			nRet = -2;
			return nRet;
		}

		for (i = 0; i < nScanChannel; i++)
		{
			nRaw = pMutulRaw[i];
			if (fSelectTXMode)
				nAddTx = 1;
			fFind = true;
			if (fSelectTXMode)
				nCheckStart = -1;
			if (pListArray[nRun][i + nAddTx] == 0 && fFind)
			{
				if ((nRaw > nShortMaxRawDefine) && (i > nCheckStart))
				{
					if (fSelectTXMode)
					{
						if (pShortSkipArray[i] != 0x01)
						{
							STX_INFO("tx i %d , raw %d", i, nRaw);
#ifdef ST_RAWTEST_LOGFILE
							snprintf(data1, 80, "TX i %d , raw %d\n", i, nRaw);
#if !FIH_GKI_MODULE_DRIVER
							kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
							//fFindShort = true;
							nRet = -1;
							if (nRaw > nMaxShortRecordRaw)
								nMaxShortRecordRaw = nRaw;
						}
					}
					else
					{
						if (pShortSkipArray[i] != 0x01)
						{
							STX_INFO("i %d , raw %d", i, nRaw);
#ifdef ST_RAWTEST_LOGFILE
							snprintf(data1, 80, "RX i %d , raw %d\n", i, nRaw);
#if !FIH_GKI_MODULE_DRIVER
							kernel_write(filp, data1, strlen(data1), ppos);
#endif
#endif
							if (nRaw > nMaxShortRecordRaw)
								nMaxShortRecordRaw = nRaw;
							//fFindShort = true;
							nRet = -1;
						}
					}
				}
			}
		}
		STX_INFO("short test result %d  nRun %d", nRet, nRun);
	}
	return nRet;
}

/*
	rtbuf is a u8 buf[X * Y + K]
	X:sensor count of X
	Y:sensor count of Y
	K:sensor count of K
	buf[?] = 0 means pass
	buf[?] = 1 means fail
	bug[20] means TX: 20/RX_count , RX: 20%RX_count
	RX_count : maybe X or Y , according layout..

	return :
	<0 error
	0  success
	>0 failed sensor count
*/
static int st_drv_test_raw(st_u8 *rtbuf, int length)
{
	st_int result;
	st_u8 buf[8];
	//st_int sensorCount = 0;
	st_int raw_J[MAX_SENSOR_COUNT];
	st_int tMode[4];
	st_int txr, rxr;
#ifdef ST_RAWTEST_LOGFILE
	struct file *filp;
	char data1[50];
	//mm_segment_t fs;
	loff_t pos;

#if !FIH_GKI_MODULE_DRIVER
	filp = filp_open(ST_RAW_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
#endif
#if FIH_GKI_MODULE_DRIVER
	filp = NULL;

	if(sizeof(raw_J) > 0)
	{
		//nothing to do
	}
#endif

	if (IS_ERR(filp))
	{
		STX_ERROR("ST open %s error...", ST_RAW_LOG_PATH);
		return -1;
	}

	//fs = get_fs();
	//set_fs(KERNEL_DS);
	pos = 0;
//	filp->f_op->llseek(filp, 0, 0);
#endif
	result = 0;
	memset(rtbuf, 0, length);
	STX_INFO("start of st_drv_test_raw");
	/* /////////////////////////// */
	/* check status */
	memset(buf, 0, 8);
	result = stx_i2c_read_bytes(1, buf, 8);
	if (result < 0)
	{
		STX_ERROR("ST I2C error (%d)", result);
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "ST I2C error (%d)\n", result);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result = -1;
		goto ST_RAW_CLOSE_FILE;
	}

	STX_INFO("status :0x%X", buf[0]);
	if ((buf[0] & 0xf) == 6)
	{
		STX_ERROR("ST IC in boot code , can't do test !");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "ST IC in boot code , can't do test !\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result = -1;
		goto ST_RAW_CLOSE_FILE;
	}

	buf[0] = 0xF1;
	buf[1] = 0x40;
	stx_i2c_write_bytes(buf, 2);
	st_msleep(100);
	/* ///////////////////////////// */
	/* get tmode */
	stx_i2c_read_bytes(0xF0, buf, 1);
	tMode[0] = (buf[0] & 0x04) >> 2; /* tx is ? */
	STX_INFO("ST TX flag = %d", tMode[0]);
	stx_i2c_read_bytes(0xF5, buf, 3);
	tMode[1] = buf[0];		 /* x */
	tMode[2] = buf[1];		 /* y */
	tMode[3] = buf[2] & 0xf; /* key */

	//sensorCount = tMode[1] + tMode[2] + tMode[3];

	STX_INFO("sensor count:%d %d %d", tMode[1], tMode[2], tMode[3]);

	memset(rtbuf, 0, tMode[1] * tMode[2] + tMode[3]);

	/* //////////////////////////// */
	/* get raw and judge */
	if(g_version_select == ST14348_VERSION)
	{

	//[CC]Add to print raw test threshold
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "MAX_RAW_LIMIT:%d\nMIN_RAW_LIMIT:%d\n", MAX_RAW_LIMIT, MIN_RAW_LIMIT);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
	STX_INFO("[CC]MAX_RAW_LIMIT:%d\n", MAX_RAW_LIMIT);
	STX_INFO("[CC]MIN_RAW_LIMIT:%d\n", MIN_RAW_LIMIT);

#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "CRITERION_RATIO_MAX:%d\nCRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MAX, CRITERION_RATIO_MIN);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		STX_INFO("[CC]CRITERION_RATIO_MAX:%d\n", CRITERION_RATIO_MAX);
		STX_INFO("[CC]CRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MIN);

#ifdef ST_RAWTEST_LOGFILE
	result = st_drv_Get_2D_RAW(tMode, raw_J, 0, rtbuf, filp,&pos);
#else
	result = st_drv_Get_2D_RAW(tMode, raw_J, 0, rtbuf);
#endif

	}else{

		//[CC]Add to print raw test threshold
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "MAX_RAW_LIMIT:%d\nMIN_RAW_LIMIT:%d\n", MAX_RAW_LIMIT_ST14348B, MIN_RAW_LIMIT_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		STX_INFO("[CC]MAX_RAW_LIMIT:%d\n", MAX_RAW_LIMIT_ST14348B);
		STX_INFO("[CC]MIN_RAW_LIMIT:%d\n", MIN_RAW_LIMIT_ST14348B);

#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "CRITERION_RATIO_MAX:%d\nCRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MAX_ST14348B, CRITERION_RATIO_MIN_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		STX_INFO("[CC]CRITERION_RATIO_MAX:%d\n", CRITERION_RATIO_MAX_ST14348B);
		STX_INFO("[CC]CRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MIN_ST14348B);

#ifdef ST_RAWTEST_LOGFILE
	result = st_drv_Get_2D_RAW_ST14348B(tMode, raw_J, 0, rtbuf, filp,&pos);
#else
	result = st_drv_Get_2D_RAW_ST14348B(tMode, raw_J, 0, rtbuf);
#endif
	}
	if (result != 0)
	{
		STX_ERROR("Error: Test fail with %d sensor", result);
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "Error: Test fail with %d sensor\n", result);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
	}
	else
	{
		STX_INFO("Test open successed!");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "Test open successed!\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result = 0;
	}
	/* //////////////////////////// */
	/* int nRawData[36]; */
	/* Get1DRaw(false,nRawData); */
	/* st_irq_off(); */
	WriteTestCFG();
#ifdef ST_RAWTEST_LOGFILE
	rxr = RunFindShortStart(false, 0x24, 0x14, 3, filp,&pos);
#else
	rxr = RunFindShortStart(false, 0x24, 0x14, 3);
#endif
	if (rxr < 0)
	{
		STX_ERROR("Error: Test Rx Short fail");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "Error: Test Rx Short fail\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result++;
	}

#ifdef ST_RAWTEST_LOGFILE
	txr = RunFindShortStart(true, 0x24, 0x14, 3, filp,&pos);
#else
	txr = RunFindShortStart(true, 0x24, 0x14, 3);
#endif
	if (txr < 0)
	{
		STX_ERROR("Error: Test Tx Short fail");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "Error: Test Tx Short fail\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result++;
	}
	if (rxr >= 0 && txr >= 0)
	{
		STX_INFO("Test short successed!");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "Test short successed!\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
	}

	/* st_irq_on(); */
	/* /////////////////////////// */
	buf[0] = 2;
	buf[1] = 1;
	stx_i2c_write_bytes(buf, 2);
	st_msleep(150);

	stx_clr_crash_msg();
ST_RAW_CLOSE_FILE:

#ifdef ST_RAWTEST_LOGFILE
	//set_fs(fs);
#if !FIH_GKI_MODULE_DRIVER
	filp_close(filp, NULL);
#endif
	STX_INFO("Test result file	: %s", ST_RAW_LOG_PATH);
#endif

	return result;
}
#endif /* ST_TEST_RAW */

int st_testraw_invoke(void)
{
	st_int ret = -1;
#ifdef ST_TEST_RAW
	st_u8 rtbuf[0x14 * 0x24];
	int length = sizeof(rtbuf) / sizeof(st_u8);
	stx_gpts.is_testing = true;
	ret = st_drv_test_raw(rtbuf, length);
	stx_gpts.is_testing = false;
#endif /* ST_TEST_RAW */

	return ret;
}

#define LCD_ID_DET2 91+46
#define LCD_ID_DET1 175+46
static int get_lcm_ID(void)
{
    STX_INFO("LCD_ID_DET1 = %d, LCD_ID_DET2 = %d", gpio_get_value(LCD_ID_DET1), gpio_get_value(LCD_ID_DET2));
    if((gpio_get_value(LCD_ID_DET1) == 0) && (gpio_get_value(LCD_ID_DET2) == 1))
    {
        STX_INFO("LCM CHIP = FL7703N");
        return 1;
    } else {
        STX_INFO("LCM CHIP = Unknown");
        return -1;
    }
}

//[CC]FIH factory functions
int ResultSelfTest = 1; // FIH self test result

void touch_selftest(void)
{
	st_int result;
	st_u8 buf[8];
	//st_int sensorCount = 0;
	st_int raw_J[MAX_SENSOR_COUNT];
	st_int tMode[4];
	st_int txr, rxr;
    int fih_st_lcm_chip_type;
#ifdef ST_RAWTEST_LOGFILE
	struct file *filp;
	char data1[100];
	//mm_segment_t fs;
	loff_t pos;
	st_u8 rtbuf[0x14 * 0x24];
	int length = sizeof(rtbuf) / sizeof(st_u8);
	bool shot_test_result = false;
	bool open_test_result = false;
	//[CC]add for get TP FW version
	char fw_version[50] = { 0 };
	//[CC]add to record TP/LCM chip type
	char str_type_title[50] = { 0 };
	char str_type[50] = { 0 };

#ifdef ST_TEST_RAW
	stx_gpts.is_testing = true;
#endif

	ResultSelfTest = 1;

#if !FIH_GKI_MODULE_DRIVER
	filp = filp_open(ST_RAW_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
#endif
#if FIH_GKI_MODULE_DRIVER
	filp = NULL;
	if(sizeof(raw_J) > 0)
	{
		//nothing to do
	}
#endif
	if (IS_ERR(filp))
	{
		STX_ERROR("ST open %s error...", ST_RAW_LOG_PATH);
		return;
	}

	//fs = get_fs();
	//set_fs(KERNEL_DS);
	pos = 0;
//	filp->f_op->llseek(filp, 0, 0);
#endif
	//pm_qos_update_request(&i2c_req, 150);
	result = 0;
	memset(rtbuf, 0, length);
    fih_st_lcm_chip_type = get_lcm_ID();
	//[CC]add for get TP FW version
	if(fih_st_lcm_chip_type > -1) {
		if(fih_st_lcm_chip_type == 0)
			snprintf(str_type, 50, "%s%s", str_type, "ST7703/");
		else
			snprintf(str_type, 50, "%s%s", str_type, "FL7703N/");
		snprintf(str_type_title, 50, "%s%s", str_type_title, "DDIC/");
	}
	if(fih_st_tp_chip_type > -1) {
		if(fih_st_tp_chip_type == 0)
			snprintf(str_type, 50, "%s%s", str_type, "ST14348/");
		else
			snprintf(str_type, 50, "%s%s", str_type, "ST14348B/");
		snprintf(str_type_title, 50, "%s%s", str_type_title, "Touch_IC/");
	}
	snprintf(str_type_title, 50, "%s%s", str_type_title, "FW_version");

	sitronix_touch_tpfwver_read(fw_version);
	if(strlen(str_type) > 0) {
		snprintf(str_type, 50, "%s%s", str_type, fw_version);
		STX_INFO("%s:%s", str_type_title, str_type);
	} else {
		STX_INFO("%s:%s", "FW_Version", fw_version);
	}
#ifdef ST_RAWTEST_LOGFILE
	if(strlen(str_type) > 0) {
		snprintf(data1, 100, "%s:%s", str_type_title, str_type);
	} else {
		snprintf(data1, 100, "%s:%s", "FW_Version", fw_version);
	}
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif

	STX_INFO("start of st_drv_test_raw");
	/* /////////////////////////// */
	/* check status */
	memset(buf, 0, 8);
	result = stx_i2c_read_bytes(1, buf, 8);
	if (result < 0)
	{
		STX_ERROR("ST I2C error (%d)", result);
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "ST I2C error (%d)\n", result);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result = -1;
		ResultSelfTest = -1;
		goto ST_RAW_CLOSE_FILE;
	}
	STX_INFO("status :0x%X", buf[0]);
	if ((buf[0] & 0xf) == 6)
	{
		STX_ERROR("ST IC in boot code , can't do test !");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "ST IC in boot code , can't do test !\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result = -1;
		goto ST_RAW_CLOSE_FILE;
	}
	buf[0] = 0xF1;
	buf[1] = 0x40;
	stx_i2c_write_bytes(buf, 2);
	st_msleep(100);
	/* ///////////////////////////// */
	/* get tmode */
	stx_i2c_read_bytes(0xF0, buf, 1);
	tMode[0] = (buf[0] & 0x04) >> 2; /* tx is ? */
	STX_INFO("ST TX flag = %d", tMode[0]);
	stx_i2c_read_bytes(0xF5, buf, 3);
	tMode[1] = buf[0];		 /* x */
	tMode[2] = buf[1];		 /* y */
	tMode[3] = buf[2] & 0xf; /* key */

	//sensorCount = tMode[1] + tMode[2] + tMode[3];
	STX_INFO("sensor count:%d %d %d", tMode[1], tMode[2], tMode[3]);

	memset(rtbuf, 0, tMode[1] * tMode[2] + tMode[3]);

	//[CC]Add to print raw test threshold
	if(g_version_select == ST14348_VERSION)
	{
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "MAX_RAW_LIMIT:%d\nMIN_RAW_LIMIT:%d\n", MAX_RAW_LIMIT, MIN_RAW_LIMIT);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
	STX_INFO("[CC]MAX_RAW_LIMIT:%d\n", MAX_RAW_LIMIT);
	STX_INFO("[CC]MIN_RAW_LIMIT:%d\n", MIN_RAW_LIMIT);

#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "CRITERION_RATIO_MAX:%d\nCRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MAX, CRITERION_RATIO_MIN);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		STX_INFO("[CC]CRITERION_RATIO_MAX:%d\n", CRITERION_RATIO_MAX);
		STX_INFO("[CC]CRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MIN);

	/* get raw and judge */
#ifdef ST_RAWTEST_LOGFILE
	result = st_drv_Get_2D_RAW(tMode, raw_J, 0, rtbuf, filp,&pos);
#else
	result = st_drv_Get_2D_RAW(tMode, raw_J, 0, rtbuf);
#endif

	} else {
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "MAX_RAW_LIMIT:%d\nMIN_RAW_LIMIT:%d\n", MAX_RAW_LIMIT_ST14348B, MIN_RAW_LIMIT_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		STX_INFO("[CC]MAX_RAW_LIMIT:%d\n", MAX_RAW_LIMIT_ST14348B);
		STX_INFO("[CC]MIN_RAW_LIMIT:%d\n", MIN_RAW_LIMIT_ST14348B);

#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 50, "CRITERION_RATIO_MAX:%d\nCRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MAX_ST14348B, CRITERION_RATIO_MIN_ST14348B);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		STX_INFO("[CC]CRITERION_RATIO_MAX:%d\n", CRITERION_RATIO_MAX_ST14348B);
		STX_INFO("[CC]CRITERION_RATIO_MIN:%d\n", CRITERION_RATIO_MIN_ST14348B);

#ifdef ST_RAWTEST_LOGFILE
	result = st_drv_Get_2D_RAW_ST14348B(tMode, raw_J, 0, rtbuf, filp,&pos);
#else
	result = st_drv_Get_2D_RAW_ST14348B(tMode, raw_J, 0, rtbuf);
#endif
	}

	/* //////////////////////////// */

	if (result != 0)
	{
		STX_ERROR("Error: Test fail with %d sensor", result);
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "Error: Test fail with %d sensor\n", result);
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
	}
	else
	{
		STX_INFO("Test open successed!");
		open_test_result = true;
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "Test open successed!\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result = 0;
	}
	/* //////////////////////////// */
	/* int nRawData[36]; */
	/* Get1DRaw(false,nRawData); */
	/* st_irq_off(); */
	WriteTestCFG();
#ifdef ST_RAWTEST_LOGFILE
	rxr = RunFindShortStart(false, 0x24, 0x14, 3, filp,&pos);
#else
	rxr = RunFindShortStart(false, 0x24, 0x14, 3);
#endif
	if (rxr < 0)
	{
		STX_ERROR("Error: Test Rx Short fail");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "Error: Test Rx Short fail\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result++;
	}

#ifdef ST_RAWTEST_LOGFILE
	txr = RunFindShortStart(true, 0x24, 0x14, 3, filp,&pos);
#else
	txr = RunFindShortStart(true, 0x24, 0x14, 3);
#endif
	if (txr < 0)
	{
		STX_ERROR("Error: Test Tx Short fail");
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "Error: Test Tx Short fail\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
		result++;
	}
	if (rxr >= 0 && txr >= 0)
	{
		STX_INFO("Test short successed!");
		shot_test_result = true;
#ifdef ST_RAWTEST_LOGFILE
		snprintf(data1, 100, "Test short successed!\n");
#if !FIH_GKI_MODULE_DRIVER
		kernel_write(filp, data1, strlen(data1), &pos);
#endif
#endif
	}

	/* st_irq_on(); */
	/* /////////////////////////// */
	buf[0] = 2;
	buf[1] = 1;
	stx_i2c_write_bytes(buf, 2);
	st_msleep(150);
	stx_clr_crash_msg();
ST_RAW_CLOSE_FILE:

#ifdef ST_RAWTEST_LOGFILE
	//set_fs(fs);

#if !FIH_GKI_MODULE_DRIVER
	filp_close(filp, NULL);
#endif
	STX_INFO("Test result file	: %s", ST_RAW_LOG_PATH);
#endif

	if(shot_test_result && open_test_result && ResultSelfTest != -1) {
		ResultSelfTest = 0;
	}

#ifdef ST_TEST_RAW
	stx_gpts.is_testing = false;
#endif
	//pm_qos_update_request(&i2c_req, PM_QOS_DEFAULT_VALUE);
	return;
}

int selftest_result_read(void)
{
	return ResultSelfTest;
}