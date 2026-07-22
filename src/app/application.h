#pragma once

namespace app {

class Application {
public:
    Application(int argc, char* argv[]);

    int run();

private:
    bool force_client_ = false;
};

}  // namespace app
