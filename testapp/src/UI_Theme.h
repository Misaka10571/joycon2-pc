#pragma once
// UI Theme - Material Design 3 inspired warm palette
#include "imgui/imgui.h"

namespace UITheme {

// Color palette — warm tones: teal primary, sage accents, warm gray surfaces
inline ImVec4 ColorFromHex(unsigned int hex, float alpha = 1.0f) {
    return ImVec4(
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f,
        alpha
    );
}

// MD3 Palette
inline const ImVec4 Primary         = ColorFromHex(0x2D6A4F);      // Deep teal green
inline const ImVec4 PrimaryHover    = ColorFromHex(0x40916C);      // Lighter teal
inline const ImVec4 PrimaryActive   = ColorFromHex(0x1B4332);      // Dark teal
inline const ImVec4 OnPrimary       = ColorFromHex(0xFFFFFF);      // White text on primary

inline const ImVec4 Surface         = ColorFromHex(0xF5F0EB);      // Warm off-white
inline const ImVec4 SurfaceCard     = ColorFromHex(0xFFFFFF);      // Pure white cards
inline const ImVec4 SurfaceVariant  = ColorFromHex(0xEDE5DB);      // Warm sand
inline const ImVec4 SurfaceDim      = ColorFromHex(0xD5CFC8);      // Dimmed surface

inline const ImVec4 Sidebar         = ColorFromHex(0xF0EBE5);      // Warm sidebar
inline const ImVec4 SidebarHover    = ColorFromHex(0xE4DFD8);      // Sidebar hover
inline const ImVec4 SidebarActive   = ColorFromHex(0xD8E8DF);      // Selected - subtle teal tint
inline const ImVec4 SidebarText     = ColorFromHex(0x49454F);      // Medium gray text

inline const ImVec4 TextPrimary     = ColorFromHex(0x1C1B1F);      // Near-black
inline const ImVec4 TextSecondary   = ColorFromHex(0x49454F);      // Medium gray
inline const ImVec4 TextTertiary    = ColorFromHex(0x79747E);      // Light gray

inline const ImVec4 Success         = ColorFromHex(0x2D6A4F);      // Green
inline const ImVec4 Error           = ColorFromHex(0xBA1A1A);      // Red
inline const ImVec4 Warning         = ColorFromHex(0xC77A20);      // Amber

inline const ImVec4 Border          = ColorFromHex(0xD5CFC8, 0.5f);// Subtle border
inline const ImVec4 Divider         = ColorFromHex(0xCAC4D0, 0.3f);// Divider

inline const ImVec4 ButtonSecondary     = ColorFromHex(0xE8E0D8);  // Muted button
inline const ImVec4 ButtonSecondaryHov  = ColorFromHex(0xDBD3CB);  // Muted button hover
inline const ImVec4 ButtonDanger        = ColorFromHex(0xF2DEDE);  // Soft red bg
inline const ImVec4 ButtonDangerHov     = ColorFromHex(0xE8C4C4);  // Red hover

inline void Apply() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding
    style.WindowRounding    = 0.0f;     // Full window — no rounding
    style.ChildRounding     = 12.0f;    // Card rounding
    style.FrameRounding     = 8.0f;     // Button/input rounding
    style.PopupRounding     = 12.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding      = 8.0f;
    style.TabRounding       = 8.0f;

    // Spacing
    style.WindowPadding     = ImVec2(0, 0);
    style.FramePadding      = ImVec2(16, 10);
    style.ItemSpacing       = ImVec2(12, 8);
    style.ItemInnerSpacing  = ImVec2(8, 4);
    style.ScrollbarSize     = 8.0f;

    // Borders
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;

    // Colors
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]            = Surface;
    c[ImGuiCol_ChildBg]             = ImVec4(0, 0, 0, 0); // transparent by default
    c[ImGuiCol_PopupBg]             = SurfaceCard;
    c[ImGuiCol_Border]              = Border;
    c[ImGuiCol_BorderShadow]        = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_FrameBg]             = SurfaceVariant;
    c[ImGuiCol_FrameBgHovered]      = SurfaceDim;
    c[ImGuiCol_FrameBgActive]       = SurfaceDim;

    c[ImGuiCol_TitleBg]             = Sidebar;
    c[ImGuiCol_TitleBgActive]       = Sidebar;
    c[ImGuiCol_TitleBgCollapsed]    = Sidebar;

    c[ImGuiCol_MenuBarBg]           = Sidebar;

    c[ImGuiCol_ScrollbarBg]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]       = SurfaceDim;
    c[ImGuiCol_ScrollbarGrabHovered]= TextTertiary;
    c[ImGuiCol_ScrollbarGrabActive] = TextSecondary;

    c[ImGuiCol_CheckMark]           = Primary;
    c[ImGuiCol_SliderGrab]          = Primary;
    c[ImGuiCol_SliderGrabActive]    = PrimaryActive;

    c[ImGuiCol_Button]              = Primary;
    c[ImGuiCol_ButtonHovered]       = PrimaryHover;
    c[ImGuiCol_ButtonActive]        = PrimaryActive;

    c[ImGuiCol_Header]              = SidebarActive;
    c[ImGuiCol_HeaderHovered]       = SidebarHover;
    c[ImGuiCol_HeaderActive]        = SidebarActive;

    c[ImGuiCol_Separator]           = Divider;
    c[ImGuiCol_SeparatorHovered]    = Primary;
    c[ImGuiCol_SeparatorActive]     = PrimaryActive;

    c[ImGuiCol_Tab]                 = Sidebar;
    c[ImGuiCol_TabHovered]          = SidebarHover;

    c[ImGuiCol_Text]                = TextPrimary;
    c[ImGuiCol_TextDisabled]        = TextTertiary;

    c[ImGuiCol_PlotLines]           = Primary;
    c[ImGuiCol_PlotHistogram]       = Primary;
}

} // namespace UITheme
