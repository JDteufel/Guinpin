#include <spdlog/fmt/fmt.h>
#include "PerformanceRAMView.h"
#include "../../../api/SystemInfoApi.h"
#include "../../../utils/UnitConverter.h"
#include "../../../storage/AppSettings.h"
#include "../../../api/process/ProcessManager.h"
#include <gtkmm/messagedialog.h>
#include <glibmm/ustring.h>
#include "../../MainWindow.h"

PerformanceRAMView::PerformanceRAMView(MainWindow *window, PerformanceButton *button)
    : PerformanceSubView(window, button, new GraphWidget(0, 100, 60)), m_AlertShown(false) {
    Gdk::RGBA ramColor;
    ramColor.set_rgba(137 / 256., 97 / 256., 153 / 256., 1.);
    button->GetGraph()->SetAxisColor(ramColor);

    m_HeadlineTitle.set_text("RAM");
    m_HeadlineDevice.set_text("");

    m_UsageGraph->SetAxisColor(ramColor);

    AddFlowDetail("Usage", m_FlowMemoryUsage);
    AddFlowDetail("Used", m_FlowUsedMemory);
    AddFlowDetail("Available", m_FlowAvailableMemory);
    AddFlowDetail("Total", m_FlowTotalMemory);

    m_UpdateThread = new DispatcherThread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            m_UpdateThread->Dispatch();
        }, [this]() {
            auto totalRam = SystemInfoApi::GetTotalRam();
            auto usedRam = SystemInfoApi::GetUsedRam();
            auto availableRam = SystemInfoApi::GetFreeRam();
            auto ramUsagePercent = SystemInfoApi::GetRamUsagePercent() * 100.;

            auto useIec = AppSettings::Get().useIECUnits;
            auto ramUnit = UnitConverter::GetBestUnitForBytes(totalRam, useIec);
            m_Button->SetInfoText(fmt::format("{:.1f}/{:.0f} {} ({:.0f} %)",
                                              UnitConverter::ConvertBytes(usedRam, ramUnit).value,
                                              UnitConverter::ConvertBytes(totalRam, ramUnit).value,
                                              UnitConverter::UnitTypeToString(ramUnit),
                                              ramUsagePercent));

            auto unit = UnitType::AUTO;
            if(useIec)
                unit = UnitType::AUTO_I;
            m_FlowMemoryUsage.set_text(fmt::format("{:.0f}%", ramUsagePercent));
            m_FlowUsedMemory.set_text(UnitConverter::ConvertBytesString(usedRam, unit));
            m_FlowAvailableMemory.set_text(UnitConverter::ConvertBytesString(availableRam, unit));
            m_FlowTotalMemory.set_text(UnitConverter::ConvertBytesString(totalRam, unit));

            m_UsageGraph->AddPoint(ramUsagePercent);
            m_Button->AddGraphPoint(ramUsagePercent);

            // Verifica y muestra alerta si corresponde
            CheckAndShowHighMemoryProcessAlert();
    });
    m_UpdateThread->Start();
}

void PerformanceRAMView::CheckAndShowHighMemoryProcessAlert() {
    auto totalRam = SystemInfoApi::GetTotalRam();
    if(totalRam <= 0) return;

    auto processes = ProcessManager::GetAllProcesses();
    ProcessNode* highProc = nullptr;
    for(auto* proc : processes) {
        if(proc->GetRAMUsage() > 0.7 * totalRam) {
            highProc = proc;
            break;
        }
    }
    if(highProc && !m_AlertShown) {
        auto unit = UnitType::AUTO;
        if(AppSettings::Get().useIECUnits)
            unit = UnitType::AUTO_I;
        std::string msg = "¡Alerta!\nEl proceso siguiente está usando más del 70% de la RAM:\n";
        msg += "PID: " + std::to_string(highProc->GetPid()) + "\n";
        msg += "Nombre: " + highProc->GetName() + "\n";
        msg += "Memoria: " + UnitConverter::ConvertBytesString(highProc->GetRAMUsage(), unit) + " / " +
               UnitConverter::ConvertBytesString(totalRam, unit);

        Gtk::MessageDialog dialog(
            *static_cast<Gtk::Window*>(m_Window),
            Glib::ustring(msg),
            false,
            Gtk::MESSAGE_WARNING,
            Gtk::BUTTONS_OK,
            true
        );
        dialog.run();
        m_AlertShown = true;
    }
    if(!highProc)
        m_AlertShown = false;
}
