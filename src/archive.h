#pragma once

extern bool g_verbose;

int archive_print_deps(const char* input);
int archive_bsp_dir(const char* input, const char* output, const char* gamedir);
int archive_bsp(const char* input, const char* output, const char* gamedir);