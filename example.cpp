#include "ecs.h"
#include <iostream>

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    std::string serialize() {
        return "my color is r: " + std::to_string(r) + " g: " + std::to_string(g) + " b: " + std::to_string(b);
    }
};


struct TimeOfDay {
    int time = 0;
};

struct print_color : System {
    void run() override {
        for (const auto& entity : entities) {
            Color* color = ecs_get_component<Color>(entity);
            std::cout << color->serialize() << '\n';
        }
    }
};

struct print_timeofday : System {
    void run() override {
        std::cout << "its " << ecs_get_resource<TimeOfDay>()->time << " o'clock\n";
    }
};

struct advance_timeofday : System {
    void run() override {
        ecs_get_resource<TimeOfDay>()->time += 1;
    }
};

struct DayNightCycle : Plugin {
    void setup() override {
        ecs_add_system<print_timeofday>();
        ecs_add_system<advance_timeofday>();

        ecs_change_system_schedule<advance_timeofday,UPDATE>();
    }
};

struct prepare_new_day : System {
    void run() override {
        ecs_get_resource<TimeOfDay>()->time = 0;
    }
};

/// SCHEDULE CALLED WHEN A NEW DAY START
struct NEWDAY{};

int main() {
    ecs_setup();
    ecs_register_component<Color>();
    ecs_add_resource<TimeOfDay>();

    ecs_add_plugin<DayNightCycle>();
    
    ecs_add_system<print_color,Color>();
    
    EntityID entity1 = ecs_add_entity();
    EntityID entity2 = ecs_add_entity();
    EntityID entity3 = ecs_add_entity();
    
    Color& entity1_color = ecs_add_component<Color>(entity1);
    entity1_color.r = 122;
    entity1_color.g = 17;
    entity1_color.b = 0;

    Color& entity2_color = ecs_add_component<Color>(entity2);
    entity2_color.r = 0;
    entity2_color.g = 245;
    entity2_color.b = 178;

    Color& entity3_color = ecs_add_component<Color>(entity3);
    entity3_color.r = 0;
    entity3_color.g = 12;
    entity3_color.b = 79;

    ecs_add_system<prepare_new_day>();
    ecs_change_system_schedule<prepare_new_day,NEWDAY>();
    ecs_change_system_schedule<print_color,NEWDAY>();

    ecs_run_systems<SETUP>();
    /// TIME SHOULD ADVANCE AND BE PRINTED 3 TIMES
    ecs_run_systems<UPDATE>();
    ecs_run_systems<UPDATE>();
    ecs_run_systems<UPDATE>();
    
    ecs_run_systems<NEWDAY>();
}

