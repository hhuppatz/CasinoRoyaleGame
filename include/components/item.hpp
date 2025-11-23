#ifndef ITEM_HPP
#define ITEM_HPP

struct item {
    bool active;
    float time_until_pickup; // Set to negative to never pickup (?)
    float time_until_despawn; // Set to negative to never despawn
};

#endif