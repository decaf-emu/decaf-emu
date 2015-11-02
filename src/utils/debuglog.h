#pragma once

template<typename... Args>
static void
debugPrint(fmt::StringRef msg, Args... args) {
   auto out = fmt::format(msg, args...);
   out += "\n";
   OutputDebugStringA(out.c_str());
}