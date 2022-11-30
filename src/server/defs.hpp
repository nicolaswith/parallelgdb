#ifndef DEFS_HPP
#define DEFS_HPP

#include <iostream>

#include <gtkmm.h>
#include "asio.hpp"

using asio::ip::tcp;
using std::string;
using std::size_t;

#define dump(var) std::cout << #var << " = " << var << "\n"
#define dumpp(var) printf("%s = %p\n", #var, (void *)var)

#endif /* DEFS_HPP */
