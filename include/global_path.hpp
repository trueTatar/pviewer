#pragma once
#if defined(Q_OS_WIN64)
  const QString g_basicPath = "C:/";
#elif defined(Q_OS_LINUX)
  inline const QString g_basicPath = "/home/user/Pictures/";
#endif