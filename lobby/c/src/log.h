#pragma once

#define err(__m, ...)		fprintf(stderr, "[%s] Error: " __m "\n", g_name, ##__VA_ARGS__);
#define log(__m, ...)		printf("[%s] " __m "\n", g_name, ##__VA_ARGS__);

