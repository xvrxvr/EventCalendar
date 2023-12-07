#pragma once
#include "text_draw.h"

bool multi_select_const(const std::string_view& data, const TextBoxDraw::TextGlobalDefinition &td, int reserved_lines=0, int x=0, int y=0, int width=RES_X, int height=RES_Y);
bool multi_select_vary(std::function<std::string_view()> new_data, const TextBoxDraw::TextGlobalDefinition &td, int reserved_lines=0, int x=0, int y=0, int width=RES_X, int height=RES_Y);

