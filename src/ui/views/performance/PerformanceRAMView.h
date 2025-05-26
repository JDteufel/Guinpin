#ifndef WSYSMON_PERFORMANCERAMVIEW_H
#define WSYSMON_PERFORMANCERAMVIEW_H

#include "PerformanceSubView.h"

class PerformanceRAMView : public PerformanceSubView {
public:
    explicit PerformanceRAMView(MainWindow *window, PerformanceButton *button);

private:
    Gtk::Label m_FlowMemoryUsage;
    Gtk::Label m_FlowUsedMemory;
    Gtk::Label m_FlowAvailableMemory;
    Gtk::Label m_FlowTotalMemory;

    void CheckAndShowHighMemoryProcessAlert(); // Method to check and show alert for high memory usage
    bool m_AlertShown = false; // Flag to avoid repeated alerts
};

#endif //WSYSMON_PERFORMANCERAMVIEW_H
