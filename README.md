# ECS-System-Cpp-Header
A header only c++ ecs system to use however ones want

## usage
Download the ecs.h file and add it to your project.

call ecs_setup() before doing anything else with it.
```cpp
int main() {
    ecs_setup();
}
```

Register your Resources and Components
```cpp
/// BOTH RESOURCES AND COMPONENTS ARE SIMPLE STRUCTS

struct TimeOfDay {
    int time = 0;
};

struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    std::string serialize() {
        return "my color is" +
        " r: " + std::to_strin(r)+
        " g: " + std::to_string(g) +
        " b: " + std::to_string(b);
    }
};

int main() {
    ...

    ecs_register_component<Color>();
    ecs_add_resource<TimeOfDay>();
}
```

Add your systems with the components they affect

~~WARNING: the systems are running in the opposite order they were added~~
```cpp

...

/// SYSTEMS ARE STRUCT WITH A void run() FUNCTION
/// AND AN ENTITYID ARRAY FILLED WITH THE ENTITIES THEY AFFECT
/// THE AFFECTED ENTITIES DEPEND ON THE COMPONENTS THE SYSTEM WAS ADDED WITH

struct print_timeofday : System {
    void run() override {
        std::cout << "its " << ecs_get_resource<TimeOfDay>()->time << " o'clock\n";
    }
};

struct print_color : System {
    void run() override {
        for (const auto& entity : entities) {
            Color* color = ecs_get_component<Color>(entity);
            std::cout << color->serialize() << '\n';
        }
    }
};

int main() {
    ...

    /// SYSTEM WILL AFFECT ENTITIES WITH A COLOR COMPONENTS
    ecs_add_system<print_color,Color>(); // RUNS SECOND
    /// SYSTEM WILL AFFECT NO ENTITIES
    ecs_add_system<print_timeofday>(); // RUNS FIRST
}
```
You can also add systems through plugins
```cpp
struct advance_timeofday : System {
    void run() override {
        ecs_get_resource<TimeOfDay>()->time += 1;
    }
};

struct print_timeofday : System {
    void run() override {
        std::cout << "its " << ecs_get_resource<TimeOfDay>()->time << " o'clock\n";
    }
};

struct DayNightCycle : Plugin {
    void setup() override {
        ecs_add_system<print_timeofday>();
        ecs_add_system<advance_timeofday>();
    }
};

int main() {
    ...

    ecs_add_plugin<DayNightCycle>();
}
```

Create your entities, add and fill the components (optional if component has a base value)
```cpp
int main() {
    ...

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
}
```
Get and update your resources
```cpp
int main() {
    ...

    ecs_get_resource<TimeOfDay>()->time = 4;
}
```
Add schedules to trigger the systems you want (default: SETUP, UPDATE)
```cpp
...
struct advance_timeofday : System {
    void run() override {
        ecs_get_resource<TimeOfDay>()->time += 1;
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
    ...
    /// CAN NOW BE USED ON A NEW SYSTEM TO TRIGGER IT WHEN ITS A NEW DAY
    ecs_add_system_schedule<NEWDAY>();

    ecs_add_system<advance_timeofday>();
    ecs_add_system<prepare_new_day>();

    /// DOES NOTHING BUT CAN BE USEFULL TO UNDERSTAND THE CODE
    ecs_change_system_schedule<advance_timeofday,UPDATE>();
    /// CHANGE prepare_new_day SCHEDULE TO THE NEWDAY SCHEDULE
    ecs_change_system_schedule<prepare_new_day,NEWDAY>();
}
```

Run your systems
```cpp
...

int main() {
    ...
    /// RUN EVERY SYSTEM WITH NON SPECIFIED OR UPDATE SCHEDULE
    ecs_run_systems();

    /// RUN EVERY SYSTEM WITH THE NEWDAY SCHEDULE
    ecs_run_systems<NEWDAY>();
}
```

## FUTURE FEATURE

- Multithreading system if that dont depend on each other

## CONTRIBUTION

Contributions are welcomed

## LICENCE MIT