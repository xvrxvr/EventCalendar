#include "common.h"
#include "web_gadgets.h"

#include <esp_heap_caps.h>

esp_err_t send_stat(httpd_req_t *req)
{
    Ans ans(req);

    ans << UTF8 << R"__(<!DOCTYPE html>
<html><title>Statistics</title>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
</head>
<h1><center>Memory statistics</center></h1>
<table class="w3-table w3-striped w3-border w3-bordered">
<tr><th>Type<th>Free<th>Allocated<th>Lagest Free Block<th>Free Lowest<th>Allocated Blocks<th>Free Blocks<th>Total Blocks</tr>
)__";

    multi_heap_info_t info;

#define K(fld) info.fld << " (" << ((info.fld + 512) >> 10) << "K)"

    auto p = [&](const char* id, uint32_t cap) {
        heap_caps_get_info(&info, cap);
        ans << UTF8 << "<tr><th>" << id << "<td>" << K(total_free_bytes) << "<td>" << K(total_allocated_bytes) << "<td>" << K(largest_free_block) << "<td>"
        << K(minimum_free_bytes) << "<td>" << info.allocated_blocks << "<td>" << info.free_blocks << "<td>" << info.total_blocks << "</tr>";
    };
#undef K    
    p("Default", MALLOC_CAP_DEFAULT);
    p("32 Bit", MALLOC_CAP_32BIT);
    p("8 Bit", MALLOC_CAP_8BIT);
    p("DMA", MALLOC_CAP_DMA);
    p("Internal", MALLOC_CAP_INTERNAL);
    p("Exec", MALLOC_CAP_EXEC);
//    p("IRAM 8 Bit", MALLOC_CAP_IRAM_8BIT);
//    p("Retention", MALLOC_CAP_RETENTION);
//    p("RTC", MALLOC_CAP_RTCRAM);
    ans << UTF8 << "</table><hr><h1><center>Tasks</center></h1><table class=\"w3-table w3-striped w3-border w3-bordered\"><tr><th>Name<th>State<th>Priority<th>Ticks<th>Stack WM</tr>";

    auto total = uxTaskGetNumberOfTasks();
    auto arr = new TaskStatus_t[total];
    uint32_t total_ticks = 0;
    total = uxTaskGetSystemState( arr, total, &total_ticks);
    std::sort(arr, arr+total, [](const TaskStatus_t& v1, const TaskStatus_t& v2) {
        int cmp = strcmp(v1.pcTaskName, v2.pcTaskName);
        if (cmp) return (cmp < 0);
        return (v1.xTaskNumber < v2.xTaskNumber);
    });
    for(int i=0; i< total; ++i)
    {
        const auto& T = arr[i];
        static const char* es[] = {"Running", "Ready", "Blocked", "Suspended", "Deleted", "Invalid"};

        ans << UTF8 << "<tr><th>" << T.pcTaskName << "<td>" << (T.eCurrentState > eInvalid ? "???" : es[T.eCurrentState]) << "<td>";
        if (T.uxCurrentPriority == T.uxBasePriority) ans << T.uxCurrentPriority; else ans << UTF8 << T.uxCurrentPriority << "[" << T.uxBasePriority << "]";
        ans << UTF8 << "<td>" << T.ulRunTimeCounter << "<td>" << T.usStackHighWaterMark << "</tr>";
    }
    double load = (2*total_ticks - (arr[0].ulRunTimeCounter + arr[1].ulRunTimeCounter)) * 50. / total_ticks;
    delete[] arr;
    ans << UTF8 << "</table>Total ticks: " << total_ticks << "<br>Load: " << load <<"% </html>";
    return ESP_OK;
}
