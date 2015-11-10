#pragma once

template<typename... Args>
static void
debugPrint(fmt::CStringRef msg, Args... args) {
   auto out = fmt::format(msg, args...);
   out += "\n";
   OutputDebugStringA(out.c_str());
}