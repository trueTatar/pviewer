#pragma once

struct NumericalOrder {
  enum ExitCode {
    FillCache = 1,
    UpdateCache = 2,
    GoThroughCache = 3,
    SetImageFromCache = 4,
    NoAction = 5
  };
  static constexpr int step = 1;
};

struct ReverseOrder {
  enum ExitCode {
    FillCache = 10,
    UpdateCache = 20,
    GoThroughCache = 30,
    SetImageFromCache = 40,
    NoAction = 50
  };
  static constexpr int step = -1;
};
