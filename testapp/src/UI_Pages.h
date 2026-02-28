#pragma once
// UI Pages - Dashboard, Add Device, Layout Manager, Mouse Settings
#include "imgui/imgui.h"
#include "i18n.h"
#include "ConfigManager.h"
#include "PlayerManager.h"
#include "DeviceManager.h"
#include "ViGEmManager.h"
#include "UI_Theme.h"
#include <string>
#include <cmath>
#include "imgui_internal.h"

// Current wizard state for Add Device
struct AddDeviceWizard {
    int step = 0;  // 0 = select type, 1 = configure, 2 = scanning, 3 = dual second scan
    ControllerType selectedType = ControllerType::SingleJoyCon;
    JoyConSide selectedSide = JoyConSide::Left;
    JoyConOrientation selectedOrientation = JoyConOrientation::Upright;
    GyroSource selectedGyro = GyroSource::Both;
    bool scanStarted = false;
    float scanTimer = 0.0f;
    std::string statusMessage;
    bool dualFirstDone = false;

    void Reset() {
        step = 0; scanStarted = false; scanTimer = 0.0f;
        statusMessage.clear(); dualFirstDone = false;
    }
};

inline AddDeviceWizard g_wizard;
inline int g_selectedLayoutIndex = 0;
inline char g_layoutNameBuf[128] = {};
inline bool g_renamingLayout = false;

// Helper: Draw a card background
inline void BeginCard(float width = 0, float minHeight = 0) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UITheme::SurfaceCard);
    ImGui::PushStyleColor(ImGuiCol_Border, UITheme::Border);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    if (width <= 0) width = ImGui::GetContentRegionAvail().x;
    float h = (minHeight > 0) ? minHeight : 0;
    ImGui::BeginChild(ImGui::GetID("card"), ImVec2(width, h), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
}

inline void EndCard() {
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// Helper: Draw a section label
inline void SectionLabel(const char* text) {
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : nullptr);
    ImGui::TextColored(UITheme::TextSecondary, "%s", text);
    if (ImGui::GetIO().Fonts->Fonts.Size > 1) ImGui::PopFont();
    ImGui::Spacing();
}

// Helper: Primary button
inline bool PrimaryButton(const char* label, ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, UITheme::Primary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UITheme::PrimaryHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UITheme::PrimaryActive);
    ImGui::PushStyleColor(ImGuiCol_Text, UITheme::OnPrimary);
    bool result = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return result;
}

// Helper: Secondary button
inline bool SecondaryButton(const char* label, ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, UITheme::ButtonSecondary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UITheme::ButtonSecondaryHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UITheme::SurfaceDim);
    ImGui::PushStyleColor(ImGuiCol_Text, UITheme::TextPrimary);
    bool result = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return result;
}

// Helper: Danger button
inline bool DangerButton(const char* label, ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, UITheme::ButtonDanger);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UITheme::ButtonDangerHov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, UITheme::Error);
    ImGui::PushStyleColor(ImGuiCol_Text, UITheme::Error);
    bool result = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);
    return result;
}

// Helper: Status chip
inline void StatusChip(const char* label, ImVec4 bg, ImVec4 textColor) {
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float pad = 8.0f;
    float chipH = textSize.y + pad * 2;
    float chipW = textSize.x + pad * 3;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + chipW, pos.y + chipH), ImGui::GetColorU32(bg), chipH * 0.5f);
    // Circle indicator
    float dotR = 4.0f;
    dl->AddCircleFilled(ImVec2(pos.x + pad + dotR, pos.y + chipH * 0.5f), dotR, ImGui::GetColorU32(textColor));
    dl->AddText(ImVec2(pos.x + pad + dotR * 2 + 6, pos.y + pad), ImGui::GetColorU32(textColor), label);
    ImGui::Dummy(ImVec2(chipW, chipH));
}

// Helper: Spinner animation
inline void Spinner(const char* label, float radius, float thickness, ImU32 color) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(radius * 2, (radius + ImGui::GetStyle().FramePadding.y) * 2);

    ImGui::Dummy(size);
    ImDrawList* dl = window->DrawList;

    float t = (float)ImGui::GetTime();
    int numSegments = 30;
    int start = (int)(fmod(t * 8.0f, (float)numSegments));

    float aMin = IM_PI * 2.0f * ((float)start) / (float)numSegments;
    float aMax = IM_PI * 2.0f * ((float)start + (float)(numSegments - 5)) / (float)numSegments;

    ImVec2 centre(pos.x + radius, pos.y + radius + ImGui::GetStyle().FramePadding.y);

    for (int i = 0; i < numSegments; i++) {
        float a0 = aMin + ((float)i / (float)numSegments) * (aMax - aMin);
        float a1 = aMin + ((float)(i + 1) / (float)numSegments) * (aMax - aMin);
        dl->AddLine(
            ImVec2(centre.x + cosf(a0) * radius, centre.y + sinf(a0) * radius),
            ImVec2(centre.x + cosf(a1) * radius, centre.y + sinf(a1) * radius),
            color, thickness);
    }
}

// Controller type icon (drawn with ImGui shapes)
inline void DrawControllerIcon(ControllerType type, ImVec2 pos, float scale, ImU32 color) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float s = scale;

    switch (type) {
    case ControllerType::SingleJoyCon: {
        // Simple rounded rectangle representing a single Joy-Con
        dl->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 16*s, pos.y + 40*s), color, 6*s);
        dl->AddCircleFilled(ImVec2(pos.x + 8*s, pos.y + 12*s), 4*s, ImGui::GetColorU32(UITheme::Surface)); // Stick
        dl->AddRectFilled(ImVec2(pos.x + 4*s, pos.y + 22*s), ImVec2(pos.x + 12*s, pos.y + 26*s), ImGui::GetColorU32(UITheme::Surface), 1*s); // Buttons
        break;
    }
    case ControllerType::DualJoyCon: {
        // Two Joy-Cons side by side
        dl->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 14*s, pos.y + 36*s), color, 5*s);
        dl->AddRectFilled(ImVec2(pos.x + 18*s, pos.y), ImVec2(pos.x + 32*s, pos.y + 36*s), color, 5*s);
        dl->AddCircleFilled(ImVec2(pos.x + 7*s, pos.y + 11*s), 3.5f*s, ImGui::GetColorU32(UITheme::Surface));
        dl->AddCircleFilled(ImVec2(pos.x + 25*s, pos.y + 11*s), 3.5f*s, ImGui::GetColorU32(UITheme::Surface));
        break;
    }
    case ControllerType::ProController: {
        // Wide rounded rectangle
        dl->AddRectFilled(ImVec2(pos.x, pos.y + 4*s), ImVec2(pos.x + 40*s, pos.y + 30*s), color, 8*s);
        dl->AddCircleFilled(ImVec2(pos.x + 12*s, pos.y + 16*s), 4*s, ImGui::GetColorU32(UITheme::Surface)); // Left stick
        dl->AddCircleFilled(ImVec2(pos.x + 28*s, pos.y + 16*s), 4*s, ImGui::GetColorU32(UITheme::Surface)); // Right stick
        break;
    }
    case ControllerType::NSOGCController: {
        // GameCube style — wider with distinctive shape
        dl->AddRectFilled(ImVec2(pos.x + 2*s, pos.y + 2*s), ImVec2(pos.x + 38*s, pos.y + 28*s), color, 10*s);
        dl->AddCircleFilled(ImVec2(pos.x + 14*s, pos.y + 14*s), 5*s, ImGui::GetColorU32(UITheme::Surface)); // Main stick
        dl->AddCircleFilled(ImVec2(pos.x + 30*s, pos.y + 12*s), 3*s, ImGui::GetColorU32(UITheme::Surface)); // C-stick
        break;
    }
    }
}

// =============================================================
// PAGE: Dashboard
// =============================================================
inline void RenderDashboard() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::BeginChild("DashboardContent", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(24, 16));

    // ViGEm Status
    if (ViGEmManager::Instance().IsConnected()) {
        StatusChip(T("dash_vigem_ok"),
            UITheme::ColorFromHex(0xD4EDDA), UITheme::Success);
    } else {
        StatusChip(T("dash_vigem_fail"),
            UITheme::ColorFromHex(0xF8D7DA), UITheme::Error);
    }

    ImGui::Spacing(); ImGui::Spacing();

    auto& pm = PlayerManager::Instance();
    int total = pm.GetPlayerCount();

    if (total == 0) {
        // Empty state
        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        float avail = ImGui::GetContentRegionAvail().x;
        float centerX = avail * 0.5f;

        // Center the empty state text
        const char* emptyText = T("dash_no_device");
        ImVec2 textSize = ImGui::CalcTextSize(emptyText);
        ImGui::SetCursorPosX(24 + centerX - textSize.x * 0.5f);
        ImGui::TextColored(UITheme::TextSecondary, "%s", emptyText);

        const char* hintText = T("dash_no_device_hint");
        textSize = ImGui::CalcTextSize(hintText);
        ImGui::SetCursorPosX(24 + centerX - textSize.x * 0.5f);
        ImGui::TextColored(UITheme::TextTertiary, "%s", hintText);
    } else {
        // Device cards
        int playerIndex = 1;

        // Single Joy-Con players
        for (int i = 0; i < (int)pm.GetSinglePlayers().size(); ++i) {
            auto& p = pm.GetSinglePlayers()[i];
            ImGui::PushID(playerIndex * 100 + i);
            BeginCard(0, 0);
            ImGui::SetCursorPos(ImVec2(16, 12));

            // Icon
            ImVec2 iconPos = ImGui::GetCursorScreenPos();
            DrawControllerIcon(ControllerType::SingleJoyCon, iconPos, 1.2f, ImGui::GetColorU32(UITheme::Primary));
            ImGui::Dummy(ImVec2(24, 48));
            ImGui::SameLine();

            // Info
            ImGui::BeginGroup();
            ImGui::Text("%s %d - %s", T("dash_player"), playerIndex, T("type_single_joycon"));
            ImGui::TextColored(UITheme::TextSecondary, "%s  |  %s: %s",
                T("dash_mapping"),
                p.side == JoyConSide::Left ? T("dash_side_left") : T("dash_side_right"),
                p.orientation == JoyConOrientation::Upright ? T("dash_orient_upright") : T("dash_orient_sideways"));
            if (p.mouseMode > 0) {
                const char* modeNames[] = { "", "FAST", "NORMAL", "SLOW" };
                ImGui::TextColored(UITheme::Warning, "Mouse: %s", modeNames[p.mouseMode]);
            }
            ImGui::EndGroup();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
            if (DangerButton(T("dash_disconnect"), ImVec2(80, 32))) {
                pm.RemovePlayerByGlobalIndex(i);
                ImGui::PopID();
                EndCard();
                break;
            }

            EndCard();
            ImGui::PopID();
            ImGui::Spacing();
            playerIndex++;
        }

        // Dual Joy-Con players
        for (int i = 0; i < (int)pm.GetDualPlayers().size(); ++i) {
            auto& p = pm.GetDualPlayers()[i];
            ImGui::PushID(playerIndex * 100 + 50 + i);
            BeginCard(0, 0);
            ImGui::SetCursorPos(ImVec2(16, 12));

            ImVec2 iconPos = ImGui::GetCursorScreenPos();
            DrawControllerIcon(ControllerType::DualJoyCon, iconPos, 1.2f, ImGui::GetColorU32(UITheme::Primary));
            ImGui::Dummy(ImVec2(40, 48));
            ImGui::SameLine();

            ImGui::BeginGroup();
            ImGui::Text("%s %d - %s", T("dash_player"), playerIndex, T("type_dual_joycon"));
            const char* gyroName = T("dash_gyro_both");
            if (p->gyroSource == GyroSource::Left) gyroName = T("dash_gyro_left");
            else if (p->gyroSource == GyroSource::Right) gyroName = T("dash_gyro_right");
            ImGui::TextColored(UITheme::TextSecondary, "%s  |  %s: %s", T("dash_mapping"), T("dash_gyro_source"), gyroName);
            ImGui::EndGroup();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
            int globalIdx = (int)pm.GetSinglePlayers().size() + i;
            if (DangerButton(T("dash_disconnect"), ImVec2(80, 32))) {
                pm.RemovePlayerByGlobalIndex(globalIdx);
                ImGui::PopID();
                EndCard();
                break;
            }

            EndCard();
            ImGui::PopID();
            ImGui::Spacing();
            playerIndex++;
        }

        // Pro / NSO GC players
        for (int i = 0; i < (int)pm.GetProPlayers().size(); ++i) {
            auto& p = pm.GetProPlayers()[i];
            ImGui::PushID(playerIndex * 100 + 80 + i);
            BeginCard(0, 0);
            ImGui::SetCursorPos(ImVec2(16, 12));

            ImVec2 iconPos = ImGui::GetCursorScreenPos();
            DrawControllerIcon(p.type, iconPos, 1.2f, ImGui::GetColorU32(UITheme::Primary));
            ImGui::Dummy(ImVec2(48, 40));
            ImGui::SameLine();

            ImGui::BeginGroup();
            const char* typeName = (p.type == ControllerType::ProController) ? T("type_pro") : T("type_nso_gc");
            ImGui::Text("%s %d - %s", T("dash_player"), playerIndex, typeName);
            ImGui::TextColored(UITheme::TextSecondary, "%s", T("dash_mapping"));
            if (p.type == ControllerType::ProController) {
                auto& config = ConfigManager::Instance().config.proConfig;
                if (!config.layouts.empty()) {
                    auto& layout = config.layouts[config.activeLayoutIndex];
                    ImGui::TextColored(UITheme::TextTertiary, "GL: %s  GR: %s  [%s]",
                        ButtonMappingToString(layout.glMapping).c_str(),
                        ButtonMappingToString(layout.grMapping).c_str(),
                        layout.name.c_str());
                }
            }
            ImGui::EndGroup();

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
            int globalIdx = (int)pm.GetSinglePlayers().size() + (int)pm.GetDualPlayers().size() + i;
            if (DangerButton(T("dash_disconnect"), ImVec2(80, 32))) {
                pm.RemovePlayerByGlobalIndex(globalIdx);
                ImGui::PopID();
                EndCard();
                break;
            }

            EndCard();
            ImGui::PopID();
            ImGui::Spacing();
            playerIndex++;
        }
    }

    ImGui::EndChild();
}

// =============================================================
// PAGE: Add Device
// =============================================================
inline void RenderAddDevice(int& activePage) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::BeginChild("AddDeviceContent", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(24, 16));

    // Step indicators
    ImGui::TextColored(UITheme::TextTertiary, "%s %d / %s",
        g_wizard.step <= 2 ? "" : "",
        g_wizard.step + 1,
        g_wizard.selectedType == ControllerType::DualJoyCon ? "4" : "3");
    ImGui::Spacing();

    if (g_wizard.step == 0) {
        // Step 1: Select controller type
        SectionLabel(T("add_step1_title"));
        ImGui::Spacing();

        struct TypeOption {
            ControllerType type;
            const char* nameKey;
        };
        TypeOption options[] = {
            { ControllerType::SingleJoyCon, "type_single_joycon" },
            { ControllerType::DualJoyCon,   "type_dual_joycon" },
            { ControllerType::ProController, "type_pro" },
            { ControllerType::NSOGCController, "type_nso_gc" }
        };

        for (auto& opt : options) {
            ImGui::PushID((int)opt.type);
            bool selected = g_wizard.selectedType == opt.type;

            ImVec4 bgCol = selected ? UITheme::SidebarActive : UITheme::SurfaceCard;
            ImVec4 borderCol = selected ? UITheme::Primary : UITheme::Border;

            ImGui::PushStyleColor(ImGuiCol_ChildBg, bgCol);
            ImGui::PushStyleColor(ImGuiCol_Border, borderCol);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, selected ? 2.0f : 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);

            ImGui::BeginChild("typecard", ImVec2(ImGui::GetContentRegionAvail().x, 64), ImGuiChildFlags_Borders);

            ImGui::SetCursorPos(ImVec2(16, 12));
            ImVec2 iconPos = ImGui::GetCursorScreenPos();
            DrawControllerIcon(opt.type, iconPos, 1.0f, ImGui::GetColorU32(selected ? UITheme::Primary : UITheme::TextSecondary));
            ImGui::Dummy(ImVec2(48, 36));
            ImGui::SameLine();
            ImGui::SetCursorPosY(20);
            ImGui::Text("%s", T(opt.nameKey));

            // Click detection
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                g_wizard.selectedType = opt.type;
            }

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
            ImGui::PopID();
            ImGui::Spacing();
        }

        ImGui::Spacing();
        if (PrimaryButton(T("add_next"), ImVec2(120, 40))) {
            // Pro and NSO GC skip step 2 config
            if (g_wizard.selectedType == ControllerType::ProController ||
                g_wizard.selectedType == ControllerType::NSOGCController) {
                g_wizard.step = 2;
            } else {
                g_wizard.step = 1;
            }
        }

    } else if (g_wizard.step == 1) {
        // Step 2: Configure
        SectionLabel(T("add_step2_title"));
        ImGui::Spacing();

        if (g_wizard.selectedType == ControllerType::SingleJoyCon) {
            // Side selection
            ImGui::Text("%s", T("add_select_side"));
            if (ImGui::RadioButton(T("dash_side_left"), g_wizard.selectedSide == JoyConSide::Left))
                g_wizard.selectedSide = JoyConSide::Left;
            ImGui::SameLine();
            if (ImGui::RadioButton(T("dash_side_right"), g_wizard.selectedSide == JoyConSide::Right))
                g_wizard.selectedSide = JoyConSide::Right;

            ImGui::Spacing();

            // Orientation
            ImGui::Text("%s", T("add_select_orient"));
            if (ImGui::RadioButton(T("dash_orient_upright"), g_wizard.selectedOrientation == JoyConOrientation::Upright))
                g_wizard.selectedOrientation = JoyConOrientation::Upright;
            ImGui::SameLine();
            if (ImGui::RadioButton(T("dash_orient_sideways"), g_wizard.selectedOrientation == JoyConOrientation::Sideways))
                g_wizard.selectedOrientation = JoyConOrientation::Sideways;

        } else if (g_wizard.selectedType == ControllerType::DualJoyCon) {
            // Gyro source
            ImGui::Text("%s", T("add_select_gyro"));
            if (ImGui::RadioButton(T("dash_gyro_both"), g_wizard.selectedGyro == GyroSource::Both))
                g_wizard.selectedGyro = GyroSource::Both;
            ImGui::SameLine();
            if (ImGui::RadioButton(T("dash_gyro_left"), g_wizard.selectedGyro == GyroSource::Left))
                g_wizard.selectedGyro = GyroSource::Left;
            ImGui::SameLine();
            if (ImGui::RadioButton(T("dash_gyro_right"), g_wizard.selectedGyro == GyroSource::Right))
                g_wizard.selectedGyro = GyroSource::Right;
        }

        ImGui::Spacing(); ImGui::Spacing();
        if (SecondaryButton(T("add_back"), ImVec2(100, 40))) g_wizard.step = 0;
        ImGui::SameLine();
        if (PrimaryButton(T("add_next"), ImVec2(100, 40))) g_wizard.step = 2;

    } else if (g_wizard.step == 2) {
        // Step 3: Scanning
        if (g_wizard.selectedType == ControllerType::DualJoyCon && !g_wizard.dualFirstDone) {
            SectionLabel(u8"Scan RIGHT Joy-Con / \u626b\u63cf\u53f3 Joy-Con");
        } else {
            SectionLabel(T("add_step3_title"));
        }

        ImGui::Spacing();

        if (!g_wizard.scanStarted) {
            ImGui::TextColored(UITheme::TextSecondary, "%s", T("add_scanning_hint"));
            ImGui::Spacing();
            if (PrimaryButton(T("add_start_scan"), ImVec2(160, 44))) {
                g_wizard.scanStarted = true;
                g_wizard.statusMessage.clear();

                DeviceManager::Instance().StartScan([](ConnectedJoyCon cj, ScanState state) {
                    if (state == ScanState::Found) {
                        auto& wiz = g_wizard;
                        bool ok = false;

                        if (wiz.selectedType == ControllerType::SingleJoyCon) {
                            ok = PlayerManager::Instance().AddSingleJoyCon(cj, wiz.selectedSide, wiz.selectedOrientation);
                        } else if (wiz.selectedType == ControllerType::DualJoyCon) {
                            if (!wiz.dualFirstDone) {
                                ok = PlayerManager::Instance().AddDualJoyConFirstStep(cj, wiz.selectedGyro);
                                if (ok) {
                                    wiz.dualFirstDone = true;
                                    wiz.scanStarted = false; // reset scan for second Joy-Con
                                    wiz.step = 3; // go to dual step 2
                                    return;
                                }
                            } else {
                                ok = PlayerManager::Instance().AddDualJoyConSecondStep(cj);
                            }
                        } else {
                            ok = PlayerManager::Instance().AddProOrGC(cj, wiz.selectedType);
                        }

                        if (ok) wiz.statusMessage = "OK";
                        else wiz.statusMessage = "FAIL";
                    } else if (state == ScanState::Timeout) {
                        g_wizard.statusMessage = "TIMEOUT";
                    } else if (state == ScanState::Error) {
                        g_wizard.statusMessage = "ERROR";
                    }
                });
            }
            ImGui::SameLine();
            if (SecondaryButton(T("add_back"), ImVec2(100, 44))) {
                if (g_wizard.selectedType == ControllerType::ProController ||
                    g_wizard.selectedType == ControllerType::NSOGCController)
                    g_wizard.step = 0;
                else
                    g_wizard.step = 1;
            }
        } else {
            // Scanning in progress or complete
            if (g_wizard.statusMessage.empty()) {
                // Still scanning
                Spinner("##scan", 16.0f, 3.0f, ImGui::GetColorU32(UITheme::Primary));
                ImGui::SameLine();
                ImGui::TextColored(UITheme::TextSecondary, "%s", T("add_scanning_hint"));

                ImGui::Spacing();
                if (SecondaryButton(T("add_cancel"), ImVec2(100, 40))) {
                    DeviceManager::Instance().StopScan();
                    g_wizard.Reset();
                }
            } else if (g_wizard.statusMessage == "OK") {
                // Success
                ImGui::TextColored(UITheme::Success, "%s", T("add_connected"));
                ImGui::Spacing();
                if (PrimaryButton("OK", ImVec2(80, 36))) {
                    g_wizard.Reset();
                    activePage = 0; // go to dashboard
                }
            } else if (g_wizard.statusMessage == "TIMEOUT") {
                ImGui::TextColored(UITheme::Error, "%s", T("add_timeout"));
                ImGui::Spacing();
                if (SecondaryButton(T("add_back"), ImVec2(100, 40))) g_wizard.scanStarted = false;
            } else {
                ImGui::TextColored(UITheme::Error, "Error connecting.");
                ImGui::Spacing();
                if (SecondaryButton(T("add_back"), ImVec2(100, 40))) g_wizard.scanStarted = false;
            }
        }

    } else if (g_wizard.step == 3) {
        // Step 4 (Dual JoyCon only): Scan LEFT Joy-Con
        SectionLabel(u8"Scan LEFT Joy-Con / \u626b\u63cf\u5de6 Joy-Con");
        ImGui::Spacing();

        if (!g_wizard.scanStarted) {
            ImGui::TextColored(UITheme::TextSecondary, "%s", T("add_scanning_hint"));
            ImGui::Spacing();
            if (PrimaryButton(T("add_start_scan"), ImVec2(160, 44))) {
                g_wizard.scanStarted = true;
                g_wizard.statusMessage.clear();

                DeviceManager::Instance().StartScan([&activePage](ConnectedJoyCon cj, ScanState state) {
                    if (state == ScanState::Found) {
                        bool ok = PlayerManager::Instance().AddDualJoyConSecondStep(cj);
                        g_wizard.statusMessage = ok ? "OK" : "FAIL";
                    } else if (state == ScanState::Timeout) {
                        g_wizard.statusMessage = "TIMEOUT";
                    }
                });
            }
        } else {
            if (g_wizard.statusMessage.empty()) {
                Spinner("##scan2", 16.0f, 3.0f, ImGui::GetColorU32(UITheme::Primary));
                ImGui::SameLine();
                ImGui::TextColored(UITheme::TextSecondary, "%s", T("add_scanning_hint"));
                ImGui::Spacing();
                if (SecondaryButton(T("add_cancel"), ImVec2(100, 40))) {
                    DeviceManager::Instance().StopScan();
                    g_wizard.Reset();
                }
            } else if (g_wizard.statusMessage == "OK") {
                ImGui::TextColored(UITheme::Success, "%s", T("add_connected"));
                ImGui::Spacing();
                if (PrimaryButton("OK", ImVec2(80, 36))) {
                    g_wizard.Reset();
                    activePage = 0;
                }
            } else {
                ImGui::TextColored(UITheme::Error, "%s", T("add_timeout"));
                ImGui::Spacing();
                if (SecondaryButton(T("add_back"), ImVec2(100, 40))) g_wizard.scanStarted = false;
            }
        }
    }

    ImGui::EndChild();
}

// =============================================================
// PAGE: Layout Manager
// =============================================================
inline void RenderLayoutManager() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::BeginChild("LayoutContent", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(24, 16));

    SectionLabel(T("layout_title"));

    auto& config = ConfigManager::Instance().config.proConfig;
    float totalW = ImGui::GetContentRegionAvail().x;
    float leftW = totalW * 0.35f;

    // Left panel: layout list
    ImGui::BeginChild("LayoutList", ImVec2(leftW, ImGui::GetContentRegionAvail().y - 60), ImGuiChildFlags_None);
    
    for (int i = 0; i < (int)config.layouts.size(); ++i) {
        ImGui::PushID(i);
        bool isActive = (i == config.activeLayoutIndex);
        bool isSelected = (i == g_selectedLayoutIndex);

        ImVec4 bg = isSelected ? UITheme::SidebarActive : UITheme::SurfaceCard;
        ImVec4 border = isActive ? UITheme::Primary : (isSelected ? UITheme::PrimaryHover : UITheme::Border);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, bg);
        ImGui::PushStyleColor(ImGuiCol_Border, border);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, isActive ? 2.0f : 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

        ImGui::BeginChild("layoutitem", ImVec2(leftW - 8, 50), ImGuiChildFlags_Borders);
        ImGui::SetCursorPos(ImVec2(12, 8));

        if (g_renamingLayout && isSelected) {
            ImGui::SetNextItemWidth(leftW - 40);
            if (ImGui::InputText("##rename", g_layoutNameBuf, sizeof(g_layoutNameBuf),
                ImGuiInputTextFlags_EnterReturnsTrue)) {
                config.layouts[i].name = g_layoutNameBuf;
                g_renamingLayout = false;
                ConfigManager::Instance().Save();
            }
        } else {
            ImGui::Text("%s", config.layouts[i].name.c_str());
            if (isActive) {
                ImGui::SameLine();
                ImGui::TextColored(UITheme::Primary, "[%s]", T("layout_active_badge"));
            }
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            g_selectedLayoutIndex = i;
            g_renamingLayout = false;
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::EndChild();

    // Layout list buttons
    ImGui::SetCursorPosX(24);
    if (PrimaryButton(T("layout_add"), ImVec2(0, 36))) {
        GLGRLayout newLayout;
        newLayout.name = "Layout " + std::to_string(config.layouts.size() + 1);
        config.layouts.push_back(newLayout);
        g_selectedLayoutIndex = (int)config.layouts.size() - 1;
        ConfigManager::Instance().Save();
    }
    ImGui::SameLine();
    if (SecondaryButton(T("layout_rename"), ImVec2(0, 36)) && g_selectedLayoutIndex < (int)config.layouts.size()) {
        g_renamingLayout = true;
        strncpy_s(g_layoutNameBuf, config.layouts[g_selectedLayoutIndex].name.c_str(), sizeof(g_layoutNameBuf) - 1);
    }
    ImGui::SameLine();
    if (DangerButton(T("layout_delete"), ImVec2(0, 36)) && config.layouts.size() > 1) {
        config.layouts.erase(config.layouts.begin() + g_selectedLayoutIndex);
        if (config.activeLayoutIndex >= (int)config.layouts.size())
            config.activeLayoutIndex = (int)config.layouts.size() - 1;
        if (g_selectedLayoutIndex >= (int)config.layouts.size())
            g_selectedLayoutIndex = (int)config.layouts.size() - 1;
        ConfigManager::Instance().Save();
    }

    // Right panel: layout details
    ImGui::SameLine(leftW + 32);
    ImGui::BeginGroup();

    if (g_selectedLayoutIndex < (int)config.layouts.size()) {
        auto& layout = config.layouts[g_selectedLayoutIndex];

        ImGui::Spacing();

        // GL dropdown
        ImGui::Text("%s", T("layout_gl"));
        int glIdx = static_cast<int>(layout.glMapping);
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo("##gl", &glIdx, ButtonMappingNames, ButtonMappingCount)) {
            layout.glMapping = static_cast<ButtonMapping>(glIdx);
            ConfigManager::Instance().Save();
        }

        ImGui::Spacing();

        // GR dropdown
        ImGui::Text("%s", T("layout_gr"));
        int grIdx = static_cast<int>(layout.grMapping);
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo("##gr", &grIdx, ButtonMappingNames, ButtonMappingCount)) {
            layout.grMapping = static_cast<ButtonMapping>(grIdx);
            ConfigManager::Instance().Save();
        }

        ImGui::Spacing(); ImGui::Spacing();

        // Set Active
        bool isActive = (g_selectedLayoutIndex == config.activeLayoutIndex);
        if (!isActive) {
            if (PrimaryButton(T("layout_set_active"), ImVec2(160, 40))) {
                config.activeLayoutIndex = g_selectedLayoutIndex;
                ConfigManager::Instance().Save();
            }
        } else {
            ImGui::TextColored(UITheme::Success, ">> %s <<", T("layout_active_badge"));
        }
    }

    ImGui::Spacing(); ImGui::Spacing();
    ImGui::TextColored(UITheme::TextTertiary, "%s", T("layout_hint"));

    ImGui::EndGroup();

    ImGui::EndChild();
}

// =============================================================
// PAGE: Mouse Settings
// =============================================================
inline void RenderMouseSettings() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
    ImGui::BeginChild("MouseContent", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(24, 16));

    SectionLabel(T("mouse_title"));

    ImGui::TextColored(UITheme::TextTertiary, "%s", T("mouse_hint"));
    ImGui::Spacing(); ImGui::Spacing();

    auto& mouseConfig = ConfigManager::Instance().config.mouseConfig;
    bool changed = false;

    // Enable toggle
    BeginCard();
    ImGui::SetCursorPos(ImVec2(16, 12));
    if (ImGui::Checkbox(T("mouse_enable"), &mouseConfig.chatKeyEnabled))
        changed = true;
    EndCard();

    ImGui::Spacing(); ImGui::Spacing();

    // Sensitivity sliders
    BeginCard();
    ImGui::SetCursorPos(ImVec2(16, 12));
    ImGui::BeginGroup();

    ImGui::Text("%s", T("mouse_fast"));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
    if (ImGui::SliderFloat("##fast", &mouseConfig.fastSensitivity, 0.1f, 3.0f, "%.2f"))
        changed = true;

    ImGui::Spacing();

    ImGui::Text("%s", T("mouse_normal"));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
    if (ImGui::SliderFloat("##normal", &mouseConfig.normalSensitivity, 0.1f, 2.0f, "%.2f"))
        changed = true;

    ImGui::Spacing();

    ImGui::Text("%s", T("mouse_slow"));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
    if (ImGui::SliderFloat("##slow", &mouseConfig.slowSensitivity, 0.05f, 1.0f, "%.2f"))
        changed = true;

    ImGui::Spacing(); ImGui::Spacing();

    ImGui::Text("%s", T("mouse_scroll_speed"));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 100);
    if (ImGui::SliderFloat("##scroll", &mouseConfig.scrollSpeed, 5.0f, 120.0f, "%.0f"))
        changed = true;

    ImGui::EndGroup();
    EndCard();

    ImGui::Spacing(); ImGui::Spacing();

    // Current mode indicator
    BeginCard();
    ImGui::SetCursorPos(ImVec2(16, 12));
    ImGui::Text("%s: ", T("mouse_current_mode"));
    ImGui::SameLine();

    // Find current mouse mode from any single right joy-con player
    int currentMode = 0;
    for (auto& sp : PlayerManager::Instance().GetSinglePlayers()) {
        if (sp.side == JoyConSide::Right) {
            currentMode = sp.mouseMode;
            break;
        }
    }
    const char* modeKeys[] = { "mouse_mode_off", "mouse_mode_fast", "mouse_mode_normal", "mouse_mode_slow" };
    ImVec4 modeColors[] = { UITheme::TextTertiary, UITheme::Warning, UITheme::Primary, UITheme::Success };
    ImGui::TextColored(modeColors[currentMode], "%s", T(modeKeys[currentMode]));
    EndCard();

    if (changed) ConfigManager::Instance().Save();

    ImGui::EndChild();
}
