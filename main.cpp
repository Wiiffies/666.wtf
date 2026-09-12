// main.cpp — "666.wtf" style login screen + full test menu (Dear ImGui + DirectX 11)
// Build with: build.bat

#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <wincodec.h>
#include <cmath>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================================
// State
// ============================================================================
enum Page { PAGE_LOGIN = 0, PAGE_MENU };

struct AppState {
    Page page = PAGE_LOGIN;

    // login screen
    char license[128] = "";
    bool show_license = false;
    bool register_mode = false;
    bool account_created = false;
    bool auth_show_password = false;
    char username[64] = "";
    char password[64] = "";
    char confirm_password[64] = "";
    char auth_error[160] = "";
    float unlock_spin = 0.f;      // progress after clicking Unlock
    bool unlocking = false;
    bool unlock_done = false;
    bool menu_open = true;
    int ui_filter = 0;
    char ui_search[64] = "";
    float login_fade = 0.f;       // fade-in
    float login_exit = 0.f;       // 0..1 exit animation

    // menu
    int tab = 0;                  // 0 RAGE, 1 LEGIT, 2 VISUALS, 3 MISC, 4 SKINS, 5 CONFIG
    float tab_w[6] = {1,0,0,0,0,0}; // animated weights

    // RAGE
    bool r_enable = true;
    int  r_dt = 0;
    float r_fov = 12.f;
    float r_hc = 78.f;
    int  r_ps = 1;
    int  r_pitch = 1;
    int  r_yaw = 1;
    bool r_hideshots = true;
    bool r_fakeduck = false;
    bool r_dt_bool = true;
    bool r_resolver = true;
    bool r_fl = false;
    int  r_ping = 120;
    bool r_autopeek = true;
    bool r_slow = false;
    bool r_fakelag = true;
    int  r_fl_limit = 14;

    // LEGIT
    bool l_enable = true;
    bool l_silent = false;
    float l_fov = 4.5f;
    float l_smooth = 6.f;
    float l_rcs = 55.f;
    int  l_hitbox = 1;
    std::vector<int> l_trig{0, 0, 1, 0};
    int l_bt_ticks = 8;
    bool l_autofire = true;
    float l_delay = 40.f;

    // VISUALS
    bool v_box = true;
    int  v_boxstyle = 1;
    bool v_hp = true;
    bool v_name = true;
    bool v_wep = false;
    bool v_glow = true;
    float v_glowamt = 65.f;
    float v_glowcol[3] = {0.878f, 0.176f, 0.176f};
    bool v_chams = false;
    int  v_chamsmat = 0;
    bool v_hitmarker = true;
    bool v_radial = false;
    bool v_asus = false;
    float v_fov = 90.f;
    float v_x = 0.f;
    float v_y = 0.f;
    char v_wm[64] = "666.wtf";
    bool v_specs = true;

    // SKINS (visual UI demo only)
    int skin_weapon = 0;
    int skin_finish = 0;
    int skin_knife = 0;
    int skin_glove = 0;
    float skin_wear = 0.08f;
    int skin_seed = 1;
    int skin_stattrak = 0;
    bool skin_enabled = true;
    bool skin_custom_name = false;
    char skin_name[64] = "666.wtf";
    float skin_color[3] = {0.914f, 0.176f, 0.176f};

    // MISC
    bool m_bhop = true;
    int  m_bhopmode = 1;
    bool m_strafe = true;
    bool m_noduck = false;
    bool m_clantag = true;
    bool m_anims = false;
    bool m_night = false;
    float m_gravity = 800.f;
    float m_model[3] = {0.878f, 0.176f, 0.176f};
    int  m_key = 0;
    std::vector<std::string> m_denies{"None", "DM", "Retake", "Arena"};
    int  m_deny = 0;
    bool m_debug = false;
    bool m_hud_center = true;
    bool m_hud_right = true;

    // CONFIG
    int  cfg_i = 0;
    std::vector<std::string> cfgs{"default", "legit.cfg", "rage-ct", "rage-t", "hvh-main"};
    char cfg_name[64] = "";
    float cfg_prog = 0.f;
    bool cfg_busy = false;
    std::string cfg_msg;
    float cfg_msg_t = 0.f;
    std::vector<std::string> toasts;
    float toast_t = 0.f;
};
static AppState g;

static void Toast(const char* msg) { g.toasts.push_back(msg); g.toast_t = 4.f; }

// palette
static ImVec4 Accent(float a = 1.f)  { return ImVec4(0.98f, 0.29f, 0.34f, a); } // refined coral accent
static ImVec4 AccentSoft(float a = 1.f) { return ImVec4(0.62f, 0.18f, 0.23f, a); }
static ImVec4 Blue(float a = 1.f) { return ImVec4(0.34f, 0.63f, 1.0f, a); }
static ImVec4 Dimmed(float a = 1.f)  { return ImVec4(0.48f, 0.20f, 0.24f, a); }

// ============================================================================
// Theme
// ============================================================================
static void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle();
    s.WindowRounding = 16.f;
    s.ChildRounding = 12.f;
    s.FrameRounding = 8.f;
    s.PopupRounding = 10.f;
    s.GrabRounding = 5.f;
    s.ScrollbarRounding = 10.f;
    s.TabRounding = 8.f;
    s.FramePadding = ImVec2(12, 9);
    s.ItemSpacing = ImVec2(10, 9);
    s.ItemInnerSpacing = ImVec2(8, 6);
    s.WindowBorderSize = 1.f;
    s.ChildBorderSize = 1.f;
    s.PopupBorderSize = 1.f;
    s.ScrollbarSize = 12.f;
    s.GrabMinSize = 10.f;
    s.WindowPadding = ImVec2(18, 18);
    s.IndentSpacing = 18.f;
    s.Alpha = 1.f;

    ImVec4* c = s.Colors;
    auto C = [](float r, float g_, float b, float a) { return ImVec4(r, g_, b, a); };
    c[ImGuiCol_WindowBg]             = C(0.012f, 0.013f, 0.017f, 0.995f);
    c[ImGuiCol_ChildBg]              = C(0.025f, 0.027f, 0.035f, 0.98f);
    c[ImGuiCol_PopupBg]              = C(0.055f, 0.058f, 0.067f, 0.995f);
    c[ImGuiCol_Border]               = C(0.48f, 0.42f, 0.65f, 0.20f);
    c[ImGuiCol_FrameBg]              = C(0.10f, 0.10f, 0.115f, 1.f);
    c[ImGuiCol_FrameBgHovered]       = C(0.105f, 0.075f, 0.082f, 1.f);
    c[ImGuiCol_FrameBgActive]        = C(0.13f, 0.082f, 0.09f, 1.f);
    c[ImGuiCol_TitleBg]              = C(0.045f, 0.045f, 0.05f, 1.f);
    c[ImGuiCol_TitleBgActive]        = C(0.055f, 0.05f, 0.05f, 1.f);
    c[ImGuiCol_TitleBgCollapsed]     = C(0.045f, 0.045f, 0.05f, 1.f);
    c[ImGuiCol_MenuBarBg]            = C(0.06f, 0.06f, 0.066f, 1.f);
    c[ImGuiCol_ScrollbarBg]          = C(0.04f, 0.04f, 0.045f, 0.55f);
    c[ImGuiCol_ScrollbarGrab]        = C(0.22f, 0.22f, 0.24f, 1.f);
    c[ImGuiCol_ScrollbarGrabHovered] = C(0.30f, 0.28f, 0.28f, 1.f);
    c[ImGuiCol_ScrollbarGrabActive]  = Accent(0.9f);
    c[ImGuiCol_CheckMark]            = Accent();
    c[ImGuiCol_SliderGrab]           = Accent();
    c[ImGuiCol_SliderGrabActive]     = C(1.f, 0.35f, 0.35f, 1.f);
    c[ImGuiCol_Button]               = C(0.075f, 0.078f, 0.090f, 1.f);
    c[ImGuiCol_ButtonHovered]        = C(0.12f, 0.075f, 0.090f, 1.f);
    c[ImGuiCol_ButtonActive]         = AccentSoft(0.62f);
    c[ImGuiCol_Header]               = AccentSoft(0.30f);
    c[ImGuiCol_HeaderHovered]        = AccentSoft(0.48f);
    c[ImGuiCol_HeaderActive]         = AccentSoft(0.70f);
    c[ImGuiCol_Separator]            = C(1, 1, 1, 0.075f);
    c[ImGuiCol_SeparatorHovered]     = Accent(0.36f);
    c[ImGuiCol_SeparatorActive]      = Accent(0.58f);
    c[ImGuiCol_ResizeGrip]           = C(0, 0, 0, 0);
    c[ImGuiCol_ResizeGripHovered]    = Accent(0.30f);
    c[ImGuiCol_ResizeGripActive]     = Accent(0.55f);
    c[ImGuiCol_Text]                 = C(0.93f, 0.935f, 0.95f, 1.f);
    c[ImGuiCol_TextDisabled]         = C(0.48f, 0.50f, 0.56f, 1.f);
    c[ImGuiCol_TextSelectedBg]       = Accent(0.30f);
    c[ImGuiCol_ModalWindowDimBg]     = C(0, 0, 0, 0.65f);
}

// ============================================================================
// Custom widgets
// ============================================================================
static bool AccentButton(const char* label, const ImVec2& size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button, AccentSoft(0.23f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, AccentSoft(0.42f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Accent(0.55f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.93f, 0.94f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Border, Accent(0.32f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    bool ret = ImGui::Button(label, size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    return ret;
}static void SubLabel(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.58f, 1.f));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

// Clean editorial tab: a quiet rectangular cell with a precise active rule.
// It deliberately avoids the rounded-pill look used by the old navigation.
static bool FlatTab(const char* label, bool selected, float alpha = 1.f) {
    const ImVec2 text_size = ImGui::CalcTextSize(label);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 size(text_size.x + 24.f, 32.f);
    ImGui::InvisibleButton(label, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (hovered || selected) {
        const ImU32 fill = ImGui::GetColorU32(selected ? ImVec4(0.16f, 0.075f, 0.09f, 0.88f)
                                                        : ImVec4(1.f, 1.f, 1.f, 0.035f));
        dl->AddRectFilled(p, p + size, fill, 3.f);
    }
    const ImVec4 inactive = hovered ? ImVec4(0.80f, 0.81f, 0.85f, alpha)
                                    : ImVec4(0.50f, 0.52f, 0.58f, alpha);
    const ImU32 text_col = ImGui::GetColorU32(selected ? ImVec4(1.f, 0.94f, 0.95f, alpha) : inactive);
    dl->AddText(ImVec2(p.x + 12.f, p.y + 8.f), text_col, label);
    if (selected) {
        dl->AddRectFilled(ImVec2(p.x + 1.f, p.y + size.y - 2.f),
                          ImVec2(p.x + size.x - 1.f, p.y + size.y),
                          ImGui::GetColorU32(Accent(alpha)), 0.f);
    }
    return clicked;
}

static void SectionHeader(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, Accent());
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

static void SectionSpacer() { ImGui::Dummy(ImVec2(0, 6)); }

// Modern toggle row with a smooth animated switch and optional supporting description.
static bool Cbx(const char* label, bool* v, const char* desc = nullptr) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = ImGui::GetTextLineHeight();
    const float row_h = desc ? h * 2 + 12.f : h + 12.f;
    const float w = ImGui::GetContentRegionAvail().x;

    ImGui::InvisibleButton(label, ImVec2(w, row_h));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();
    if (clicked) *v = !*v;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImGuiID id = ImGui::GetItemID();
    static std::map<ImGuiID, float> toggle_mix;
    float& mix = toggle_mix[id];
    const float target = *v ? 1.f : 0.f;
    mix += (target - mix) * std::min(1.f, ImGui::GetIO().DeltaTime * 14.f);

    const float tw = 38.f, th = 21.f;
    const ImVec2 toggle_min(p.x + w - tw, p.y + (h - th) * 0.5f + 1.f);
    const ImVec2 toggle_max = toggle_min + ImVec2(tw, th);
    const ImVec4 off_col = hovered ? ImVec4(0.22f, 0.23f, 0.27f, 1.f) : ImVec4(0.14f, 0.15f, 0.18f, 1.f);
    const ImVec4 on_col = AccentSoft(0.92f);
    const ImVec4 track_col = ImLerp(off_col, on_col, mix);

    if (hovered) {
        dl->AddRectFilled(ImVec2(p.x - 7, p.y - 5), ImVec2(p.x + w + 7, p.y + row_h - 2),
                          ImGui::GetColorU32(Accent(0.045f)), 7.f);
    }
    if (hovered || mix > 0.01f) {
        dl->AddRectFilled(toggle_min - ImVec2(4.f, 4.f), toggle_max + ImVec2(4.f, 4.f),
                          ImGui::GetColorU32(Accent(0.07f + mix * 0.09f)), th * 0.5f + 4.f);
    }
    dl->AddRectFilled(toggle_min, toggle_max, ImGui::GetColorU32(track_col), th * 0.5f);
    dl->AddRect(toggle_min, toggle_max, ImGui::GetColorU32(mix > 0.5f ? Accent(0.48f) : ImVec4(1.f, 1.f, 1.f, 0.08f)), th * 0.5f, 0, 1.f);
    const float knob_x = toggle_min.x + th * 0.5f + (tw - th) * mix;
    dl->AddCircleFilled(ImVec2(knob_x, toggle_min.y + th * 0.5f), 7.f, IM_COL32(249, 250, 252, 255));
    if (mix > 0.5f)
        dl->AddCircleFilled(ImVec2(knob_x, toggle_min.y + th * 0.5f), 3.f, ImGui::GetColorU32(Accent(0.75f)));

    const ImU32 label_col = ImGui::GetColorU32(hovered ? ImVec4(1.f, 0.96f, 0.97f, 1.f) : ImGui::GetStyleColorVec4(ImGuiCol_Text));
    dl->AddText(ImVec2(p.x, p.y - 1), label_col, label);
    if (desc) dl->AddText(ImVec2(p.x, p.y + h + 4), ImGui::GetColorU32(ImVec4(0.48f, 0.50f, 0.56f, 1.f)), desc);
    return clicked;
}

// Clean slider: readable label/value row and a quiet track without a red halo or dot.
static bool SldF(const char* label, float* v, float mn, float mx, const char* fmt = "%.1f") {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = ImGui::GetTextLineHeight();
    const float track_h = 5.f;
    const float total_h = h + 18.f;

    ImGui::InvisibleButton(label, ImVec2(w, total_h));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    if (active)
        *v = std::clamp((ImGui::GetIO().MousePos.x - p.x) / w, 0.f, 1.f) * (mx - mn) + mn;
    *v = std::clamp(*v, mn, mx);

    const float t = (mx > mn) ? (*v - mn) / (mx - mn) : 0.f;
    char vt[64]; snprintf(vt, sizeof vt, fmt, *v);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 tsz = ImGui::CalcTextSize(vt);
    const ImU32 label_col = ImGui::GetColorU32(hovered ? ImVec4(1.f, 0.96f, 0.97f, 1.f)
                                                           : ImGui::GetStyleColorVec4(ImGuiCol_Text));
    const ImU32 value_col = ImGui::GetColorU32(active ? ImVec4(0.92f, 0.93f, 0.96f, 1.f)
                                                      : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    dl->AddText(p, label_col, label);
    dl->AddText(ImVec2(p.x + w - tsz.x, p.y), value_col, vt);

    const float ty = p.y + h + 9.f;
    const ImVec2 track_min(p.x, ty);
    const ImVec2 track_max(p.x + w, ty + track_h);
    dl->AddRectFilled(track_min, track_max, IM_COL32(255, 255, 255, 20), 3.f);
    if (t > 0.001f)
        dl->AddRectFilled(track_min, ImVec2(p.x + w * t, track_max.y), ImGui::GetColorU32(Accent(0.86f)), 3.f);

    // Small neutral handle only — no red glow/dot around the slider.
    const float gx = p.x + w * t;
    dl->AddRectFilled(ImVec2(gx - 2.5f, ty - 3.f), ImVec2(gx + 2.5f, ty + track_h + 3.f),
                      IM_COL32(242, 243, 246, 255), 2.f);
    return active;
}

static bool SldI(const char* label, int* v, int mn, int mx) {
    float f = (float)*v;
    const bool active = SldF(label, &f, (float)mn, (float)mx, "%.0f");
    *v = std::clamp((int)(f + 0.5f), mn, mx);
    return active;
}

// dropdown (combo) styled dark
static bool Cmb(const char* label, int* item, const std::vector<std::string>& items) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.09f, 0.10f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.13f, 0.10f, 0.10f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Accent(0.35f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, 0.08f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    const std::string preview = (*item >= 0 && *item < (int)items.size()) ? items[*item] : "";
    bool open = ImGui::BeginCombo(label, preview.c_str());
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    bool changed = false;
    if (open) {
        for (int i = 0; i < (int)items.size(); i++) {
            const bool sel = (*item == i);
            ImGui::PushStyleColor(ImGuiCol_Text, sel ? Accent() : ImGui::GetStyleColorVec4(ImGuiCol_Text));
            if (ImGui::Selectable(items[i].c_str(), sel)) { *item = i; changed = true; }
            ImGui::PopStyleColor();
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

// multi-select combo
static bool MultiCmb(const char* label, std::vector<int>& sel, const std::vector<std::string>& items) {
    std::string preview;
    int n = 0;
    for (int i = 0; i < (int)items.size(); i++)
        if (sel[i]) { if (n++) preview += ", "; preview += items[i]; }
    if (preview.empty()) preview = "None";
    bool open = ImGui::BeginCombo(label, preview.c_str());
    bool changed = false;
    if (open) {
        for (int i = 0; i < (int)items.size(); i++) {
            ImGui::PushID(i);
            bool on = sel[i] != 0;
            if (Cbx(items[i].c_str(), &on)) { sel[i] = on ? 1 : 0; changed = true; }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return changed;
}

// keybind button: click, then press any key. Stores ImGuiKey ids, 1000+m for mouse.
static const char* KeyName(int k) {
    static char buf[32];
    if (k == 0) return "None";
    if (k >= 1000) {
        static const char* mn[5] = {"MOUSE1", "MOUSE2", "MOUSE3", "MOUSE4", "MOUSE5"};
        return mn[k - 1000];
    }
    const char* n = ImGui::GetKeyName((ImGuiKey)k);
    if (n && *n) {
        snprintf(buf, sizeof buf, "%s", n);
        for (char* c = buf; *c; ++c) *c = (char)toupper((unsigned char)*c);
        return buf;
    }
    snprintf(buf, sizeof buf, "KEY %d", k);
    return buf;
}

static bool Keybind(const char* label, int* vk) {
    static std::map<std::string, bool> listening;
    bool listening_now = listening[label];
    ImGui::PushID(label);
    ImGui::PushStyleColor(ImGuiCol_Button, listening_now ? Accent(0.25f) : ImVec4(0.09f, 0.09f, 0.10f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text, listening_now ? Accent() : ImGui::GetStyleColorVec4(ImGuiCol_Text));
    const std::string txt = listening_now ? std::string("[ ... ]") : std::string(KeyName(*vk));
    const bool clicked = ImGui::Button(txt.c_str(), ImVec2(110, 0));
    ImGui::PopStyleColor(2);
    if (clicked) listening_now = listening[label] = true;
    if (listening_now) {
        for (ImGuiKey k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k = (ImGuiKey)(k + 1)) {
            if (k == ImGuiKey_Escape) continue;
            if (ImGui::IsKeyDown(k)) { *vk = (int)k; listening[label] = false; break; }
        }
        for (int m = 0; m < 5; m++) {
            if (ImGui::IsMouseDown(m)) { *vk = 1000 + m; listening[label] = false; break; }
        }
    }
    ImGui::PopID();
    return clicked;
}

// ============================================================================
// Draw helpers: logo, pills, header
// ============================================================================
static void StatusPill(const char* text, bool online = true) {
    ImVec2 sp = ImGui::GetCursorScreenPos();
    const ImVec2 tsz = ImGui::CalcTextSize(text);
    const float pad_x = 16.f, h = 26.f;
    const ImVec2 sz(tsz.x + pad_x * 2 + 10, h);
    const ImVec2 end = sp + sz;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 col = online ? IM_COL32(0x2e, 0xcc, 0x40, 255) : IM_COL32(0xE9, 0x2D, 0x2D, 255);
    dl->AddRectFilled(sp, end, IM_COL32(0, 0, 0, 130), sz.y * 0.5f);
    dl->AddRect(sp, end, online ? IM_COL32(0x2e, 0xcc, 0x40, 80) : IM_COL32(0xE9, 0x2D, 0x2D, 80), sz.y * 0.5f);
    dl->AddCircleFilled(ImVec2(sp.x + pad_x, sp.y + h * 0.5f), 3.5f, col);
    dl->AddText(ImVec2(sp.x + pad_x + 12, sp.y + (h - tsz.y) * 0.5f), col, text);
    ImGui::Dummy(sz);
}

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11ShaderResourceView* g_logoTexture = nullptr;
static int g_logoWidth = 0;
static int g_logoHeight = 0;

static ImFont* g_font = nullptr;      // regular 15
static ImFont* g_font_big = nullptr;  // 26 wordmark
static ImFont* g_font_logo = nullptr; // heavy logo wordmark
static ImFont* g_font_small = nullptr;// 12 subtitle

// Recreated logo treatment: heavy silver wordmark with a fine coral outline.
// The two-pass outline keeps it crisp on the black editorial canvas.
static void DrawLogo(ImVec2 top, float alpha, float scale = 1.f, bool subtitle = true) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float cx = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() * 0.5f;
    const float fs = 37.f * scale, sub_fs = 11.f * scale;
    const char* word = "666.wtf";
    ImFont* font = g_font_logo ? g_font_logo : g_font_big;
    const float word_w = font ? font->CalcTextSizeA(fs, FLT_MAX, 0, word).x : ImGui::CalcTextSize(word).x;
    const ImVec2 p(cx - word_w * 0.5f, top.y);
    const ImU32 outline = IM_COL32(0xD9, 0x28, 0x32, (int)(220.f * alpha));
    const ImU32 silver = IM_COL32(238, 240, 244, (int)(255.f * alpha));
    // layered offsets create the red edge seen in the supplied logo
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            if (x || y) dl->AddText(font, fs, p + ImVec2((float)x, (float)y), outline, word);
    dl->AddText(font, fs, p, silver, word);
    // cool-gray lower offset gives the mark a metallic depth
    dl->AddText(font, fs, p + ImVec2(0.f, 1.5f), IM_COL32(165, 168, 175, (int)(90.f * alpha)), word);
    dl->AddText(font, fs, p - ImVec2(0.f, 1.f), IM_COL32(255, 255, 255, (int)(110.f * alpha)), word);
    if (subtitle) {
        const char* sub = "C O U N T E R - S T R I K E   2";
        const float subw = g_font_small ? g_font_small->CalcTextSizeA(sub_fs, FLT_MAX, 0, sub).x : 120.f;
        dl->AddText(g_font_small, sub_fs, ImVec2(cx - subw * 0.5f, top.y + fs + 10.f * scale),
                    IM_COL32(130, 133, 142, (int)(220.f * alpha)), sub);
    }
}

static bool LoadLogoTexture(const wchar_t* path) {
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    bool ok = false;

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory)))) return false;
    if (SUCCEEDED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                                       WICDecodeMetadataCacheOnLoad, &decoder)) &&
        SUCCEEDED(decoder->GetFrame(0, &frame)) &&
        SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
        SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                        WICBitmapDitherTypeNone, nullptr, 0.0,
                                        WICBitmapPaletteTypeCustom))) {
        UINT w = 0, h = 0;
        converter->GetSize(&w, &h);
        std::vector<unsigned char> pixels((size_t)w * h * 4);
        if (SUCCEEDED(converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data()))) {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = pixels.data(); data.SysMemPitch = w * 4;
            ID3D11Texture2D* texture = nullptr;
            if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &data, &texture))) {
                D3D11_SHADER_RESOURCE_VIEW_DESC view = {};
                view.Format = desc.Format;
                view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                view.Texture2D.MipLevels = 1;
                ok = SUCCEEDED(g_pd3dDevice->CreateShaderResourceView(texture, &view, &g_logoTexture));
                texture->Release();
                if (ok) { g_logoWidth = (int)w; g_logoHeight = (int)h; }
            }
        }
    }
    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();
    return ok;
}

static void DrawImageLogo(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha = 1.f) {
    if (!g_logoTexture) return;
    // Crop the supplied square image to its centered wordmark.
    dl->AddImage(g_logoTexture, min, max, ImVec2(0.075f, 0.365f), ImVec2(0.925f, 0.675f),
                 IM_COL32(255, 255, 255, (int)(255.f * alpha)));
}

static void DrawCompactBrand(ImVec2 p, float alpha = 1.f) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (g_logoTexture) {
        DrawImageLogo(dl, p, p + ImVec2(78.f, 28.f), alpha);
        return;
    }
    ImFont* font = g_font_logo ? g_font_logo : g_font_big;
    const float fs = 18.f;
    const char* word = "666.wtf";
    const ImU32 edge = IM_COL32(0xD9, 0x28, 0x32, (int)(220.f * alpha));
    const ImU32 fill = IM_COL32(236, 238, 243, (int)(255.f * alpha));
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            if (x || y) dl->AddText(font, fs, p + ImVec2((float)x, (float)y), edge, word);
    dl->AddText(font, fs, p, fill, word);
}

// ============================================================================
// Login page
// ============================================================================
static void DrawLogin(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 disp = io.DisplaySize;

    // animate entrance
    g.login_fade = std::min(1.f, g.login_fade + dt * 2.2f);
    const float a = g.login_fade;

    // background
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(ImVec2(0, 0), disp, IM_COL32(7, 7, 8, 255));
    dl->AddRectFilledMultiColor(ImVec2(0, 0), disp,
        IM_COL32(16, 13, 13, 255), IM_COL32(9, 9, 10, 255),
        IM_COL32(7, 7, 8, 255), IM_COL32(10, 9, 9, 255));

    // wide black editorial canvas, matching the main interface
    const float cw = 720.f, ch = 500.f;
    const ImVec2 cp((disp.x - cw) * 0.5f, (disp.y - ch) * 0.5f);
    dl->AddRectFilled(cp, cp + ImVec2(cw, ch), IM_COL32(2, 3, 5, (int)(250.f * a)), 16.f);
    dl->AddRect(cp, cp + ImVec2(cw, ch), IM_COL32(105, 89, 135, (int)(80.f * a)), 16.f, 0, 1.f);
    // fine top rule, deliberately understated
    dl->AddRectFilled(ImVec2(cp.x + 20, cp.y + 1), ImVec2(cp.x + cw - 20, cp.y + 2),
                      IM_COL32(0xD9, 0x28, 0x32, (int)(210 * a)), 1.f);

    // interactable layer
    ImGui::SetNextWindowPos(cp + ImVec2(42, 24));
    ImGui::SetNextWindowSize(ImVec2(cw - 84, ch - 48));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, a);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 7));
    ImGui::Begin("#login", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground |
                                     ImGuiWindowFlags_NoMove);
    // Keep the access page focused: the old editorial utility links were removed.
    ImGui::Spacing();
    if (g_logoTexture) {
        const float logo_w = 310.f;
        const float logo_h = 116.f;
        const ImVec2 logo_pos = ImGui::GetCursorScreenPos() + ImVec2((ImGui::GetWindowWidth() - logo_w) * 0.5f, 0.f);
        DrawImageLogo(ImGui::GetWindowDrawList(), logo_pos, logo_pos + ImVec2(logo_w, logo_h), a);
        ImGui::Dummy(ImVec2(logo_w, logo_h));
    } else {
        DrawLogo(ImGui::GetCursorScreenPos(), a, 1.08f, true);
        ImGui::Dummy(ImVec2(0, 70));
    }

    // A single quiet status line keeps the form focused and uncluttered.
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.43f, 0.80f, 0.56f, a));
        ImGui::TextUnformatted("ONLINE");
        ImGui::PopStyleColor();
    }
    ImGui::Dummy(ImVec2(0, 16));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.945f, 0.96f, a));
    ImGui::TextUnformatted(g.register_mode ? "Create your account" : "Welcome back");
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.50f, 0.56f, a));
    ImGui::TextUnformatted(g.register_mode ? "Use your license key to activate access." : "Log in to continue to your workspace.");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 8));

    // auth mode switch: compact squared navigation with a clear active rule.
    {
        ImGui::BeginGroup();
        if (FlatTab("LOG IN", !g.register_mode, a)) {
            g.register_mode = false;
            g.auth_error[0] = 0;
        }
        ImGui::SameLine(0, 3);
        if (FlatTab("CREATE ACCOUNT", g.register_mode, a)) {
            g.register_mode = true;
            g.auth_error[0] = 0;
        }
        ImGui::EndGroup();
    }
    ImGui::Dummy(ImVec2(0, 10));

    // account fields
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.115f, 0.95f));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##authuser", "Username", g.username, sizeof(g.username));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##authpass", "Password", g.password, sizeof(g.password), ImGuiInputTextFlags_Password);
    if (g.register_mode) {
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##authconfirm", "Confirm password", g.confirm_password, sizeof(g.confirm_password), ImGuiInputTextFlags_Password);
    }
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 8));

    // License is required only during registration. Login is intentionally just username + password.
    if (g.register_mode) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.62f, 0.65f, 0.90f * a));
        ImGui::TextUnformatted("LICENSE KEY");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 3));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.115f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.105f, 0.125f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.11f, 0.13f, 1.f));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##license", "XXXX-XXXX-XXXX-XXXX", g.license, sizeof(g.license),
                                 g.show_license ? 0 : ImGuiInputTextFlags_Password);
        ImGui::PopStyleColor(3);
        ImGui::Dummy(ImVec2(0, 6));
    }

    // login helper link keeps the login flow visually complete without a license field
    if (!g.register_mode) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.51f, 0.57f, a));
        ImGui::TextUnformatted("Forgot your password?");
        ImGui::SameLine(0, 5);
        ImGui::PushStyleColor(ImGuiCol_Text, Accent(a));
        ImGui::TextUnformatted("Reset it");
        if (ImGui::IsItemClicked()) Toast("Password reset is a local demo action");
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 5));
    }

    // "Don't have a key? Purchase access"
    {
        ImGui::Dummy(ImVec2(0, 8));
        const char* p1 = "Need access? ";
        const char* p2 = "Purchase a key";
        const float w1 = ImGui::CalcTextSize(p1).x, w2 = ImGui::CalcTextSize(p2).x;
        const float total = w1 + w2;
        const float x = (ImGui::GetWindowWidth() - total) * 0.5f;
        ImGui::SetCursorPosX(x);
        ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.48f, a), "%s", p1);
        ImGui::SameLine(0, 0);
        ImGui::PushStyleColor(ImGuiCol_Text, Accent(a));
        ImGui::TextUnformatted(p2);
        if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
        if (ImGui::IsItemClicked()) Toast("Opening store... purchase access");
        ImGui::PopStyleColor();
    }
    ImGui::Dummy(ImVec2(0, 14));

    // Unlock button
    {
        const float bw = ImGui::GetWindowWidth();
        const char* action = g.register_mode ? "Create account" : "Log in";
        if (AccentButton(g.unlocking ? "Checking..." : action, ImVec2(bw, 40)) && !g.unlocking) {
            g.auth_error[0] = 0;
            g.unlocking = true;
            g.unlock_spin = 0.f;
        }
        if (g.unlocking) {
            g.unlock_spin += dt * 0.9f;
            if (g.unlock_spin >= 1.f) {
                g.unlocking = false;
                if (!g.username[0] || !g.password[0]) {
                    snprintf(g.auth_error, sizeof(g.auth_error), "Enter your username and password");
                } else if (g.register_mode && !g.license[0]) {
                    snprintf(g.auth_error, sizeof(g.auth_error), "A license key is required to register");
                } else if (g.register_mode && strcmp(g.password, g.confirm_password) != 0) {
                    snprintf(g.auth_error, sizeof(g.auth_error), "Passwords do not match");
                } else if (g.register_mode) {
                    g.account_created = true;
                    g.register_mode = false;
                    g.auth_error[0] = 0;
                    Toast("Account created — log in to continue");
                } else {
                    g.unlock_done = true;
                }
            }
        }
        if (g.auth_error[0]) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.42f, 0.46f, a));
            ImGui::TextWrapped("%s", g.auth_error);
            ImGui::PopStyleColor();
        }
    }

    // divider "OR"
    {
        ImGui::Dummy(ImVec2(0, 16));
        const float w = ImGui::GetWindowWidth();
        const float y = ImGui::GetCursorScreenPos().y + 8;
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        const float lx = ImGui::GetWindowPos().x;
        wdl->AddLine(ImVec2(lx, y), ImVec2(lx + w * 0.5f - 22, y), ImGui::GetColorU32(ImGuiCol_Separator));
        wdl->AddLine(ImVec2(lx + w * 0.5f + 22, y), ImVec2(lx + w, y), ImGui::GetColorU32(ImGuiCol_Separator));
        const char* or_t = "OR";
        ImGui::PushFont(g_font_small);
        const float ow = ImGui::CalcTextSize(or_t).x;
        ImGui::SetCursorPosX((w - ow) * 0.5f);
        ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.45f, a), "%s", or_t);
        ImGui::PopFont();
    }
    ImGui::Dummy(ImVec2(0, 12));

    // bottom row: game chip + buy a key
    {
        const char* game = "Counter-Strike 2";
        const float game_w = ImGui::CalcTextSize(game).x + 30;
        ImGui::SetCursorPosX(4);
        const ImVec2 pp = ImGui::GetCursorScreenPos();
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        wdl->AddRect(pp + ImVec2(0, 1), pp + ImVec2(20, 21), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.15f)), 4.f);
        wdl->AddText(g_font_small, 11.f, pp + ImVec2(6.5f, 3.5f), ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.72f, a)), "+");
        wdl->AddText(ImVec2(pp.x + 26, pp.y + 2), ImGui::GetColorU32(ImVec4(0.62f, 0.62f, 0.65f, a)), game);
        ImGui::Dummy(ImVec2(game_w, 22));

        const char* buy = g.register_mode ? "Need a license?" : "New here? Create an account";
        const float bw2 = ImGui::CalcTextSize(buy).x + 26.f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - bw2);
        ImGui::PushStyleColor(ImGuiCol_Text, Accent(a));
        ImGui::TextUnformatted(buy);
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsItemClicked()) Toast("Opening key store...");
        ImGui::SameLine(0, 4);
        ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.48f, a), "->");
        ImGui::PopStyleColor();
    }

    // spinner while checking
    if (g.unlocking) {
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        const float t = (float)ImGui::GetTime();
        const ImVec2 c(ImGui::GetWindowPos().x + ImGui::GetWindowWidth() * 0.5f,
                       ImGui::GetWindowPos().y + ImGui::GetWindowHeight() - 24.f);
        for (int i = 0; i < 8; i++) {
            const float ang = t * 5.f + i * (6.28318f / 8.f);
            wdl->AddCircleFilled(c + ImVec2(cosf(ang) * 8.f, sinf(ang) * 8.f), 2.f,
                                 ImGui::GetColorU32(Accent(0.25f + 0.75f * (i / 8.f))));
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(3);

    if (g.unlock_done) {
        g.unlock_done = false;
        g.page = PAGE_MENU;
    }
}

// ============================================================================
// Menu page
// ============================================================================
static const char* kTabs[6] = {"RAGE", "LEGIT", "VISUALS", "MISC", "SKINS", "CONFIG"};
static const char* kTabDescriptions[6] = {
    "Combat automation and targeting",
    "Precision assistance with control",
    "Player, world, and screen visuals",
    "Movement, utility, and keybinds",
    "Weapon finishes and loadout preview",
    "Profiles, presets, and storage"
};

static bool NavItem(const char* label, int index) {
    const bool selected = g.tab == index;
    ImGui::PushStyleColor(ImGuiCol_Header, selected ? Accent(0.16f) : ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Accent(0.11f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, Accent(0.22f));
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.08f, 0.5f));
    const bool clicked = ImGui::Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 38));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    if (selected) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        dl->AddRectFilled(ImVec2(min.x, min.y + 8), ImVec2(min.x + 3, max.y - 8), ImGui::GetColorU32(Accent()), 2.f);
    }
    return clicked;
}

static void GroupBox(const char* title) {
    ImGui::BeginChild(title, ImVec2(0, 0), ImGuiChildFlags_Border);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.94f, 0.945f, 0.96f, 1.f));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, Accent(0.32f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}
static void EndGroup() { ImGui::EndChild(); }

static void DrawMenu(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 disp = io.DisplaySize;

    // background
    ImDrawList* fdl = ImGui::GetBackgroundDrawList();
    fdl->AddRectFilled(ImVec2(0, 0), disp, IM_COL32(7, 7, 8, 255));
    fdl->AddRectFilledMultiColor(ImVec2(0, 0), disp,
        IM_COL32(14, 15, 21, 255), IM_COL32(8, 9, 13, 255),
        IM_COL32(6, 7, 10, 255), IM_COL32(10, 9, 14, 255));
    // subtle ambient accent, kept very low contrast for a clean surface
    fdl->AddCircleFilled(ImVec2(disp.x * 0.82f, disp.y * 0.16f), 260.f, IM_COL32(100, 22, 36, 13));
    fdl->AddCircleFilled(ImVec2(disp.x * 0.12f, disp.y * 0.88f), 220.f, IM_COL32(26, 70, 125, 10));

    // Small live status HUD. Center stats and right-side connection status are
    // independent so each can be enabled from MISC > Interface.
    // Values are intentionally local/demo values; no game process is accessed.
    {
        ImDrawList* hud = ImGui::GetForegroundDrawList();
        const float pulse = 0.55f + 0.45f * sinf((float)ImGui::GetTime() * 2.0f);
        const float chip_y = 8.f;

        if (g.m_hud_center) {
            char center_text[96];
            snprintf(center_text, sizeof center_text, "FPS %.0f   |   PING %d ms   |   TICK 64", io.Framerate, 38 + (int)(pulse * 3.f));
            const ImVec2 center_size = ImGui::CalcTextSize(center_text);
            const float center_x = (disp.x - center_size.x) * 0.5f;
            const ImVec2 center_min(center_x - 14.f, chip_y);
            const ImVec2 center_max(center_x + center_size.x + 14.f, chip_y + 22.f);
            hud->AddRectFilled(center_min, center_max, IM_COL32(10, 10, 11, 220), 6.f);
            hud->AddRect(center_min, center_max, IM_COL32(0xE9, 0x2D, 0x2D, 70), 6.f);
            hud->AddCircleFilled(ImVec2(center_min.x + 9.f, chip_y + 11.f), 3.f,
                                 ImGui::GetColorU32(Accent(0.65f + pulse * 0.3f)));
            hud->AddText(ImVec2(center_min.x + 18.f, chip_y + 4.f), IM_COL32(225, 225, 228, 235), center_text);
        }

        if (g.m_hud_right) {
            const char* right_text = "ONLINE  •  666.WTF";
            const ImVec2 right_size = ImGui::CalcTextSize(right_text);
            const float right_x = disp.x - right_size.x - 34.f;
            const ImVec2 right_min(right_x - 14.f, chip_y);
            const ImVec2 right_max(disp.x - 14.f, chip_y + 22.f);
            hud->AddRectFilled(right_min, right_max, IM_COL32(10, 10, 11, 220), 6.f);
            hud->AddRect(right_min, right_max, IM_COL32(0x2E, 0xCC, 0x40, 75), 6.f);
            hud->AddCircleFilled(ImVec2(right_min.x + 9.f, chip_y + 11.f), 3.f, IM_COL32(0x2E, 0xCC, 0x40, 255));
            hud->AddText(ImVec2(right_min.x + 18.f, chip_y + 4.f), IM_COL32(190, 225, 195, 235), right_text);
        }
    }

    if (!g.menu_open) {
        fdl->AddText(ImVec2(20, 40), IM_COL32(200, 200, 205, 150),
                     "Menu hidden - press INSERT to show");
        return;
    }

    // window
    const float W = 940.f, H = 560.f;
    ImGui::SetNextWindowPos(ImVec2((disp.x - W) * 0.5f, (disp.y - H) * 0.5f), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGui::SetNextWindowSizeConstraints(ImVec2(760, 500), ImVec2(1400, 900));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::Begin("##666menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 wp = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();
    dl->AddRectFilled(ImVec2(wp.x + 14, wp.y + 1), ImVec2(wp.x + ws.x - 14, wp.y + 3),
                      IM_COL32(0xE9, 0x2D, 0x2D, 220), 1.f);

    // ---------------- editorial black canvas ----------------
    ImGui::BeginChild("##maincontent", ImVec2(-1, -1), ImGuiChildFlags_None, ImGuiWindowFlags_None);
    const ImVec2 brand_pos = ImGui::GetCursorScreenPos();
    DrawCompactBrand(brand_pos);
    ImGui::Dummy(ImVec2(78.f, 22.f));
    ImGui::SameLine(0, 24);

    for (int i = 0; i < 6; ++i) {
        ImGui::PushID(i);
        if (FlatTab(kTabs[i], g.tab == i)) g.tab = i;
        ImGui::PopID();
        if (i < 5) ImGui::SameLine(0, 8);
    }
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 92.f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.44f, 0.78f, 0.56f, 1.f));
    ImGui::TextUnformatted("READY");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.57f, 0.58f, 0.64f, 1.f));
    ImGui::TextUnformatted("666.wtf  /  A modern control interface for your setup");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.11f, 0.12f, 1.f));
    ImGui::InputTextWithHint("##uisearch", "Search settings...", g.ui_search, sizeof(g.ui_search));
    ImGui::PopStyleColor();
    ImGui::Spacing();
    const char* filters[] = {"All", "Core", "Visual", "Input", "Profiles"};
    for (int i = 0; i < 5; ++i) {
        ImGui::PushID(100 + i);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.11f, 0.11f, 0.12f, 0.82f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, AccentSoft(0.38f));
        ImGui::PushStyleColor(ImGuiCol_Text, g.ui_filter == i ? ImVec4(1.f, 0.94f, 0.95f, 1.f) : ImVec4(0.60f, 0.61f, 0.66f, 1.f));
        if (ImGui::SmallButton(filters[i])) g.ui_filter = i;
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (i < 4) ImGui::SameLine(0, 5);
    }
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.55f, 0.45f, 0.68f, 0.22f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.955f, 0.97f, 1.f));
    ImGui::TextUnformatted(kTabs[g.tab]);
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 10);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.43f, 0.45f, 0.52f, 1.f));
    ImGui::TextUnformatted(kTabDescriptions[g.tab]);
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // ---------------- content ----------------

    switch (g.tab) {
    case 0: { // ---------------- RAGE ----------------
        ImGui::Columns(2, "#ragecols", false);
        GroupBox("Aim");
        Cbx("Enable rage bot", &g.r_enable, "Master switch for all rage features");
        Cbx("Double tap", &g.r_dt_bool);
        Cbx("Hide shots", &g.r_hideshots, "Masks your shots on other clients");
        Cbx("Resolver", &g.r_resolver, "Anti-aim correction");
        Cbx("Fake latency", &g.r_fl, "Ping spike abuse");
        if (g.r_fl) SldI("Ping amount", &g.r_ping, 0, 200);
        EndGroup();
        ImGui::NextColumn();
        GroupBox("Exploits");
        Cbx("Auto peek", &g.r_autopeek, "Quick peek & return to cover");
        Cbx("Slow walk", &g.r_slow);
        Cbx("Fake duck", &g.r_fakeduck);
        SectionSpacer();
        SldF("Field of view", &g.r_fov, 0.f, 180.f, "%.0f deg");
        SldF("Hit chance", &g.r_hc, 0.f, 100.f, "%.0f%%");
        Cmb("Point scale", &g.r_ps, {"Lowest", "Low", "Medium", "High", "Highest"});
        EndGroup();
        ImGui::Columns(1);
        break;
    }
    case 1: { // ---------------- LEGIT ----------------
        ImGui::Columns(2, "#legitcols", false);
        GroupBox("Aim assist");
        Cbx("Enable legit bot", &g.l_enable);
        Cbx("Silent aim", &g.l_silent, "Aim is not shown on your screen");
        Cbx("Auto fire", &g.l_autofire);
        SldF("FOV", &g.l_fov, 0.5f, 30.f, "%.1f deg");
        SldF("Smoothing", &g.l_smooth, 1.f, 20.f, "%.1f");
        SldF("Recoil control", &g.l_rcs, 0.f, 100.f, "%.0f%%");
        SldF("Click delay", &g.l_delay, 0.f, 400.f, "%.0f ms");
        Cmb("Hitbox", &g.l_hitbox, {"Head", "Nearest", "Chest", "Body"});
        EndGroup();
        ImGui::NextColumn();
        GroupBox("Trigger bot");
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
        MultiCmb("Trigger on", g.l_trig, {"Enemies", "Allies", "Walls ( penetration )", "Smoke check"});
        ImGui::PopStyleVar();
        EndGroup();
        GroupBox("Backtrack");
        static bool bt = true;
        Cbx("Enable backtrack", &bt, "Rewind player positions");
        SldI("Backtrack ticks", &g.l_bt_ticks, 0, 64);
        EndGroup();
        ImGui::Columns(1);
        break;
    }
    case 2: { // ---------------- VISUALS ----------------
        ImGui::Columns(3, "#viscols", false);
        GroupBox("ESP");
        Cbx("Bounding box", &g.v_box);
        if (g.v_box) Cmb("Box style", &g.v_boxstyle, {"Normal", "Corner", "3D", "2D x 2D"});
        Cbx("Health bar", &g.v_hp);
        Cbx("Names", &g.v_name);
        Cbx("Weapons", &g.v_wep);
        Cbx("Spectator list", &g.v_specs);
        EndGroup();
        ImGui::NextColumn();
        GroupBox("Models");
        Cbx("Glow", &g.v_glow);
        if (g.v_glow) {
            SldF("Glow amount", &g.v_glowamt, 0.f, 100.f, "%.0f%%");
            ImGui::TextUnformatted("Color");
            ImGui::SetNextItemWidth(-1);
            ImGui::ColorEdit3("##glowcol", g.v_glowcol, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        }
        Cbx("Chams", &g.v_chams);
        if (g.v_chams) Cmb("Material", &g.v_chamsmat, {"Flat", "Textured", "Metallic", "Glow pulse"});
        EndGroup();
        ImGui::NextColumn();
        GroupBox("Effects");
        Cbx("Hit marker", &g.v_hitmarker);
        Cbx("Radial radar", &g.v_radial);
        Cbx("World color mod", &g.v_asus, "Nightmode / asus walls");
        SldF("View FOV", &g.v_fov, 60.f, 130.f, "%.0f");
        SldF("View offset X", &g.v_x, -30.f, 30.f, "%.1f");
        SldF("View offset Y", &g.v_y, -30.f, 30.f, "%.1f");
        ImGui::TextUnformatted("Watermark text");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##wmtext", g.v_wm, sizeof(g.v_wm));
        EndGroup();
        ImGui::Columns(1);
        break;
    }
    case 3: { // ---------------- MISC ----------------
        ImGui::Columns(2, "#misccols", false);
        GroupBox("Movement");
        Cbx("Bunny hop", &g.m_bhop);
        if (g.m_bhop) Cmb("Mode", &g.m_bhopmode, {"Legit", "Rage", "Ideal jump"});
        Cbx("Auto strafe", &g.m_strafe);
        Cbx("No duck cooldown", &g.m_noduck);
        SldF("Gravity", &g.m_gravity, 200.f, 2000.f, "%.0f");
        EndGroup();
        GroupBox("Keybinds");
        ImGui::TextUnformatted("Menu key");
        ImGui::SameLine(140);
        Keybind("##menukey", &g.m_key);
        Cmb("Deny modes", &g.m_deny, g.m_denies);
        EndGroup();
        ImGui::NextColumn();
        GroupBox("Interface");
        Cbx("Center performance HUD", &g.m_hud_center, "FPS, ping, and tickrate");
        Cbx("Right status HUD", &g.m_hud_right, "Online state and branding");
        Cbx("Debug log", &g.m_debug, "Show local diagnostic messages");
        EndGroup();
        GroupBox("Misc");
        Cbx("Custom clantag", &g.m_clantag);
        Cbx("Thirdperson animations", &g.m_anims);
        Cbx("Nightmode", &g.m_night);
        ImGui::TextUnformatted("Model color");
        ImGui::SetNextItemWidth(-1);
        ImGui::ColorEdit3("##modelcol", g.m_model, ImGuiColorEditFlags_NoInputs);
        EndGroup();
        ImGui::Columns(1);
        break;
    }
    case 4: { // ---------------- SKINS ----------------
        static const std::vector<std::string> weapons{
            "AK-47", "M4A1-S", "AWP", "USP-S", "Glock-18", "Desert Eagle", "MP9", "Karambit"
        };
        static const std::vector<std::string> finishes{
            "Crimson Web", "Redline", "Printstream", "Fade", "Dragon Lore", "Doppler", "Slate", "Default"
        };
        static const std::vector<std::string> knives{
            "Karambit", "M9 Bayonet", "Butterfly", "Skeleton", "Talon", "Default"
        };
        static const std::vector<std::string> gloves{
            "Sport Gloves", "Specialist Gloves", "Driver Gloves", "Hand Wraps", "Default"
        };

        ImGui::Columns(2, "#skinscols", false);
        GroupBox("Weapon skin");
        Cbx("Enable skin changer", &g.skin_enabled, "Applies the selected preview locally");
        Cmb("Weapon", &g.skin_weapon, weapons);
        Cmb("Finish", &g.skin_finish, finishes);
        SldF("Wear", &g.skin_wear, 0.00f, 1.00f, "%.2f");
        SldI("Pattern seed", &g.skin_seed, 1, 1000);
        Cmb("Knife", &g.skin_knife, knives);
        Cmb("Gloves", &g.skin_glove, gloves);
        EndGroup();

        ImGui::NextColumn();
        GroupBox("Details");
        Cmb("StatTrak", &g.skin_stattrak, {"Disabled", "Enabled"});
        Cbx("Custom name", &g.skin_custom_name);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##skinname", "Name tag", g.skin_name, sizeof(g.skin_name));
        ImGui::TextUnformatted("Accent color");
        ImGui::SetNextItemWidth(-1);
        ImGui::ColorEdit3("##skincolor", g.skin_color, ImGuiColorEditFlags_NoInputs);
        ImGui::Spacing();
        if (AccentButton("Apply preview", ImVec2(-1, 32))) {
            Toast("Skin preview applied");
        }
        if (AccentButton("Reset selected", ImVec2(-1, 32))) {
            g.skin_wear = 0.08f;
            g.skin_seed = 1;
            g.skin_stattrak = 0;
            Toast("Skin settings reset");
        }
        EndGroup();
        ImGui::Columns(1);
        break;
    }
    case 5: { // ---------------- CONFIG ----------------
        ImGui::Columns(2, "#cfgcols", false);
        GroupBox("Configs");
        Cmb("Config", &g.cfg_i, g.cfgs);
        ImGui::Spacing();
        if (AccentButton("Load", ImVec2((ImGui::GetContentRegionAvail().x - 10) * 0.5f, 32))) {
            g.cfg_busy = true; g.cfg_prog = 0.f; g.cfg_msg = "Loaded " + g.cfgs[g.cfg_i];
        }
        ImGui::SameLine();
        if (AccentButton("Save", ImVec2(-1, 32))) {
            g.cfg_busy = true; g.cfg_prog = 0.f; g.cfg_msg = "Saved " + g.cfgs[g.cfg_i];
        }
        if (g.cfg_busy) {
            g.cfg_prog += dt * 1.5f;
            // fake progress bar with red fill
            ImVec2 pp = ImGui::GetCursorScreenPos();
            float pw = ImGui::GetContentRegionAvail().x;
            ImDrawList* wdl = ImGui::GetWindowDrawList();
            wdl->AddRectFilled(pp, pp + ImVec2(pw, 6), IM_COL32(255, 255, 255, 18), 3.f);
            wdl->AddRectFilled(pp, pp + ImVec2(pw * std::min(1.f, g.cfg_prog), 6), ImGui::GetColorU32(Accent()), 3.f);
            ImGui::Dummy(ImVec2(pw, 10));
            if (g.cfg_prog >= 1.f) { g.cfg_busy = false; Toast(g.cfg_msg.c_str()); }
        }
        EndGroup();
        GroupBox("Create new");
        ImGui::TextUnformatted("Name");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##cfgname", "my-config", g.cfg_name, sizeof(g.cfg_name));
        ImGui::Spacing();
        if (AccentButton("Create", ImVec2(-1, 30))) {
            if (g.cfg_name[0]) {
                g.cfgs.push_back(g.cfg_name);
                g.cfg_i = (int)g.cfgs.size() - 1;
                g.cfg_name[0] = 0;
                Toast("Config created");
            } else Toast("Type a name first");
        }
        EndGroup();
        ImGui::NextColumn();
        GroupBox("About");
        ImGui::Text("666.wtf test menu");
        SubLabel("ImGui + DirectX 11 UI demo");
        ImGui::Spacing();
        ImGui::Text("Frames: %.0f", io.Framerate);
        ImGui::Text("Active tab: %s", kTabs[g.tab]);
        EndGroup();
        ImGui::Columns(1);
        break;
    }
    }

    ImGui::EndChild(); // maincontent

    // The content child owns its scrolling area; do not paint a footer over the last setting.

    ImGui::End();
    ImGui::PopStyleVar();
}

// ============================================================================
// Toasts
// ============================================================================
static void DrawToasts(float dt) {
    if (g.toasts.empty()) return;
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 disp = io.DisplaySize;
    g.toast_t -= dt;
    float y = 44.f;
    for (auto it = g.toasts.rbegin(); it != g.toasts.rend(); ++it) {
        const ImVec2 tsz = ImGui::CalcTextSize(it->c_str());
        const float w = tsz.x + 40.f, h = 36.f;
        const float x = disp.x - w - 20.f;
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(12, 12, 13, 235), 8.f);
        dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(0xE9, 0x2D, 0x2D, 90), 8.f);
        dl->AddRectFilled(ImVec2(x + 6, y + 8), ImVec2(x + 8, y + h - 8), ImGui::GetColorU32(Accent()), 2.f);
        dl->AddText(ImVec2(x + 18, y + (h - tsz.y) * 0.5f), IM_COL32(230, 230, 232, 240), it->c_str());
        y += h + 8.f;
    }
    if (g.toast_t <= 0.f) {
        g.toasts.clear();
        g.toast_t = 0.f;
    }
}

// ============================================================================
// Win32 / DX11 boilerplate (official ImGui example, slightly adapted)
// ============================================================================
static ID3D11DeviceContext* g_pd3dContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRTV = nullptr;
static HWND g_hWnd = nullptr;
static bool g_done = false;

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

int main(int, char**) {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr,
                       nullptr, nullptr, nullptr, L"666wtf", nullptr };
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(0, wc.lpszClassName, L"666.wtf", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1024, 640, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hWnd) return 1;

    BOOL dark = TRUE;
    DwmSetWindowAttribute(g_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    if (!CreateDeviceD3D(g_hWnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ApplyTheme();

    // fonts: Segoe UI at three sizes (fallback to embedded default)
    {
        const char* candidates[] = {
            "C:\\Windows\\Fonts\\segoeui.ttf",
            "C:\\Windows\\Fonts\\arial.ttf",
            "C:\\Windows\\Fonts\\tahoma.ttf",
        };
        auto load = [&](float px) -> ImFont* {
            ImFontConfig cfg;
            cfg.OversampleH = 2; cfg.OversampleV = 2;
            for (const char* path : candidates) {
                if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
                    if (ImFont* f = io.Fonts->AddFontFromFileTTF(path, px, &cfg)) return f;
                }
            }
            return io.Fonts->AddFontDefault();
        };
        g_font = load(15.f);
        g_font_big = load(25.f);
        g_font_logo = load(42.f);
        g_font_small = load(12.f);
    }

    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);
    // Load the supplied logo if it is next to the executable, otherwise use the fallback wordmark.
    LoadLogoTexture(L"666-logo.png");

    auto last = std::chrono::steady_clock::now();
    while (!g_done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_done = true;
        }
        if (g_done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - last).count();
        last = now;

        if (ImGui::IsKeyPressed(ImGuiKey_Insert)) g.menu_open = !g.menu_open;

        if (g.page == PAGE_LOGIN) DrawLogin(dt);
        else DrawMenu(dt);
        DrawToasts(dt);

        ImGui::Render();
        const float clear_color[4] = { 0.027f, 0.027f, 0.031f, 1.f };
        g_pd3dContext->OMSetRenderTargets(1, &g_mainRTV, nullptr);
        g_pd3dContext->ClearRenderTargetView(g_mainRTV, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (g_logoTexture) { g_logoTexture->Release(); g_logoTexture = nullptr; }
    CoUninitialize();
    CleanupDeviceD3D();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.SampleDesc.Count = 1;
    UINT createFlags = 0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createFlags,
                                               nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                          
                                               &g_pd3dDevice, nullptr, &g_pd3dContext);
    if (FAILED(hr)) return false;
    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dContext) { g_pd3dContext->Release(); g_pd3dContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRTV);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_mainRTV) { g_mainRTV->Release(); g_mainRTV = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        g_done = true;
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
