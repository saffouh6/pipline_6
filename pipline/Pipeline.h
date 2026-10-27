#pragma once
#include <functional>
#include <vector>

// Pipeline sinifi. Sadece sirasiyla open(), produce() ve close() cagirir.
class Pipeline {
private:
    std::function<void()> runLogic;

public:
    // Builder tarafindan olusturulan calistirma mantigini alir
    explicit Pipeline(std::function<void()> logic) : runLogic(logic) {}

    void run() {
        if (runLogic) {
            runLogic();
        }
    }
};