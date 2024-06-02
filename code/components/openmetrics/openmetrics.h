#ifndef OPENMETRICS_H
#define OPENMETRICS_H

#include <string>
#include <vector>

#include "ClassFlowDefineTypes.h"

std::string createMetric(const std::string &metricName, const std::string &help, const std::string &type, const std::string &value);
std::string createSequenceMetrics(std::string prefix, const std::vector<NumberPost *> &numbers);

void register_openmetrics_uri(httpd_handle_t server);

#endif // OPENMETRICS_H
