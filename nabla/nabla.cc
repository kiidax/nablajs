/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

#include <nabla/nabla.hh>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <cstdlib>
#include <cstring>
#ifndef NO_GETOPT_LONG
#include <getopt.h>
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

extern void run_test();

static const char* prompt_string = PACKAGE "> ";
static const char* startup_message =
  PACKAGE_STRING "\n"
  "Copyright (C) 2014 Katsuya Iida.\n"
  "\n";
static const char* usage_message =
  "Usage: " PACKAGE " [OPTION]... [FILE]...\n"
  "Evaluate JavaScript code, interactively or from a script.\n"
  "\n"
  "  -h, --help     display this help and exit\n"
  "  -v, --version  display version information and exit\n"
  "\n"
  "Report bugs to: " PACKAGE_BUGREPORT "\n"
  PACKAGE_NAME " home page: <" PACKAGE_URL ">\n"
  ;
static const char* version_message =
  PACKAGE " (" PACKAGE_NAME ") " PACKAGE_VERSION "\n"
  "Copyright (C) 2014 Katsuya Iida.\n"
  "\n"
  "License GPLv3+: GNU GPL 3 or later <http://gnu.org/licenses/gpl.html>.\n"
  "This is free software: you are free to change and redistribute it.\n"
  "There is NO WARRANTY, to the extent permitted by law.\n"
  ;

static void show_meminfo()
{
  nabla::meminfo info;
  nabla::getmeminfo(&info);
  std::cerr << "Before GC: Heap size: " << info.heap_size << ", free bytes: " << info.free_bytes << std::endl;
  nabla::gc();
  nabla::getmeminfo(&info);
  std::cerr << "After GC:  Heap size: " << info.heap_size << ", free bytes: " << info.free_bytes << std::endl;
}

static void usage()
{
  std::cout << usage_message << std::flush;
  exit(EXIT_SUCCESS);
}

static void version()
{
  std::cout << version_message << std::flush;
  exit(EXIT_SUCCESS);
}

// Execute a program
static int run(nabla::context& c, const char *path) {
  std::u16string name(path, path + strlen(path));
  std::fstream fin(path);
  if (!fin)
    return -1;
  std::u16string source((std::istreambuf_iterator<char>(fin)),
                   (std::istreambuf_iterator<char>()));
  fin.close();

  std::u16string r;
  (void)c.eval(source, name, r);
#if 0
  any_ref v = c->eval(ss);
  std::cerr << "Result: " << v << std::endl;
  std::cerr << "Dumping global object..." << std::endl;
  dumpMap(c->global->props());
#endif
  return 0;
}

void interactive(nabla::context& c) {
  const char *stdin_name = "[stdin]";
  std::u16string name(stdin_name, stdin_name + strlen(stdin_name));
  std::cout << startup_message << std::flush;
#if HAVE_LIBREADLINE
  char* input;
  for (;;) {
    input = readline(prompt_string);
    if (!input)
      break;
    std::u16string source(input, input + strlen(input));
    source += static_cast<char16_t>(';');
    std::u16string r;
    if (c.eval(source, name, r)) {
      std::cout << std::string(r.begin(), r.end()) << std::endl;
    }
    add_history(input);
    free(input);
  }
  std::cout << std::endl;
#else
  for (;;) {
    std::string input;
    std::cout << prompt_string << std::flush;
    if (std::cin.eof())
      break;
    getline(std::cin, input);
    std::u16string source(input.begin(), input.end());
    source += static_cast<char16_t>(';');
    std::u16string r;
    if (c.eval(source, name, r)) {
      std::cout << std::string(r.begin(), r.end()) << std::endl;
    }
    if (std::cin.eof())
      break;
  }
#endif
}

#ifdef NO_GETOPT_LONG
int optind = 1;
#endif

int main(int argc, char* argv[])
{
  int interactive_flag = 0;

#ifndef NO_GETOPT_LONG
  while (true) {
    static struct option long_options[] = {
      { "help",    no_argument, 0, 'h' },
      { "version", no_argument, 0, 'v' },
      { 0, 0, 0, 0 }
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "hv", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 0:
      break;
    case 'h':
      usage();
      break;
    case 'v':
      version();
      break;
    default:
      exit(1);
    }
  }
#endif
  
  nabla::init();
  run_test();
  if (false) show_meminfo();
  if (optind == argc) {
    interactive_flag = 1;
  }

  nabla::context c;

  while (optind < argc) {
    const char *path = argv[optind++];
    if (run(c, path) == -1) {
      std::cerr << path << ": I/O error" << std::endl;
      exit(1);
    }
  }

  if (interactive_flag) {
    interactive(c);
  }

  return EXIT_SUCCESS;
}
