/*
	This file is part of ParallelGDB.

	Copyright (c) 2023 by Nicolas With

	ParallelGDB is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ParallelGDB is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ParallelGDB.  If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

#ifndef DEFS_HPP
#define DEFS_HPP

#include <iostream>

#include <gtkmm.h>
#include "asio.hpp"

using asio::ip::tcp;
using std::string;
using std::size_t;
using std::cout;
using std::endl;

#define dump(var) std::cout << #var << " = " << var << "\n"
#define dumpp(var) printf("%s = %p\n", #var, (void *)var)
#define dumpb(var) std::cout << #var << " = " << std::boolalpha << var << "\n"

#endif /* DEFS_HPP */
