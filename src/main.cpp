#include <iostream>
#include "App.h"

int main(void)
{   
    App app = App(640, 480);
    if (app.init() == -1) {
        std::cout << "App init err" << std::endl;
        return -1;
    }
    app.loop();
    app.~App(); // destroy
    return 0;
}