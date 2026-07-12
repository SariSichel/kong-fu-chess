#pragma once

#include <iosfwd>
#include <memory>
#include <string>

namespace texttests {

class ScriptRunner {
public:
    ScriptRunner();
    ~ScriptRunner();

    ScriptRunner(const ScriptRunner&) = delete;
    ScriptRunner& operator=(const ScriptRunner&) = delete;

    void run(std::istream& in, std::ostream& out);

private:
    void processLine(const std::string& line, std::ostream& out);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace texttests
