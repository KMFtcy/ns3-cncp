/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */
#ifndef TIME_PRINTER_H
#define TIME_PRINTER_H

#include <ostream>

/**
 * @file
 * @ingroup time
 * Declaration of ns3::TimePrinter function pointer type
 * and ns3::DefaultTimePrinter function.
 */

namespace ns3
{

/**
 * Function signature for features requiring a time formatter,
 * such as logging or ShowProgress.
 *
 * A TimePrinter should write the current simulation time
 * (Simulator::Now()) on the designated output stream.
 *
 * @param [in,out] os The output stream to print on.
 */
typedef void (*TimePrinter)(std::ostream& os);

/**
 * Default Time printer.
 *
 * @param [in,out] os The output stream to print on.
 */
void DefaultTimePrinter(std::ostream& os);

} // namespace ns3

#endif /* TIME_H */
