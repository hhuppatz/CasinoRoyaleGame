#include "systems/collision_detection_system.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "components/network.hpp"
#include "systems/jump_system.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <cmath>
#include "help_functions.hpp"
#include "components/entity_state.hpp"

void collision_detection_system::update(jump_system& jump_system) {
    // First pass: Update hitboxes to align with current transform
    // NOTE: We update hitboxes for ALL entities (including remote ones) because
    // other systems (like inventory_system) need accurate hitbox positions
    for (auto& entity : entities) {
        auto& entity_state_comp = g_conductor.get_component<entity_state>(entity);
        auto& transform_comp = g_conductor.get_component<transform>(entity);
        auto& rigidbody_comp = g_conductor.get_component<rigidbody>(entity);
        if (!entity_state_comp.is_active || !rigidbody_comp.can_collide) {
            continue;
        }

        // Sync hitbox with transform: position and scale
        rigidbody_comp.Hitbox.setPosition({transform_comp.position[0], transform_comp.position[1]});
        // Calculate actual size by multiplying base_size by scale
        float actualWidth = rigidbody_comp.base_size[0] * transform_comp.scale[0];
        float actualHeight = rigidbody_comp.base_size[1] * transform_comp.scale[1];
        rigidbody_comp.Hitbox.setSize({actualWidth, actualHeight});
    }

    // Second pass: Collision detection and resolution
    // Only check entities that have moved, or pairs where at least one has moved
    for (auto it1 = entities.begin(); it1 != entities.end(); ++it1) {
        auto& entity1 = *it1;
        auto& transform1 = g_conductor.get_component<transform>(entity1);
        auto& rigidbody1 = g_conductor.get_component<rigidbody>(entity1);
        auto& entity_state_comp1 = g_conductor.get_component<entity_state>(entity1);
        if (!entity_state_comp1.is_active || !rigidbody1.can_collide) {
            continue;
        }
        
        // Only apply collision to LOCAL entities (not remote networked entities)
        if (g_conductor.has_component<network>(entity1)) {
            auto& network_comp = g_conductor.get_component<network>(entity1);
            if (!network_comp.is_local) {
                continue; // Skip remote entities
            }
        }

        // Check if entity1 has moved
        float lastX1 = transform1.last_position[0];
        float lastY1 = transform1.last_position[1];
        float x1 = transform1.position[0];
        float y1 = transform1.position[1];
        bool entity1Moved = (x1 != lastX1 || y1 != lastY1);

        // Calculate actual size by multiplying base_size by scale
        float w1 = rigidbody1.base_size[0] * transform1.scale[0];
        float h1 = rigidbody1.base_size[1] * transform1.scale[1];

        for (auto it2 = std::next(it1); it2 != entities.end(); ++it2) {
            auto& entity2 = *it2;
            auto& transform2 = g_conductor.get_component<transform>(entity2);
            auto& rigidbody2 = g_conductor.get_component<rigidbody>(entity2);
            auto& entity_state_comp2 = g_conductor.get_component<entity_state>(entity2);
            if (!entity_state_comp2.is_active || !rigidbody2.can_collide) {
                continue;
            }
            
            // Check if entity2 has moved
            float lastX2 = transform2.last_position[0];
            float lastY2 = transform2.last_position[1];
            float x2 = transform2.position[0];
            float y2 = transform2.position[1];
            bool entity2Moved = (x2 != lastX2 || y2 != lastY2);

            // Skip collision check if neither entity has moved (both are stationary)
            if (!entity1Moved && !entity2Moved) {
                continue;
            }
            
            // Calculate actual size by multiplying base_size by scale
            float w2 = rigidbody2.base_size[0] * transform2.scale[0];
            float h2 = rigidbody2.base_size[1] * transform2.scale[1];

            // Check if the two hitboxes intersect at current position
            if (rectanglesIntersect(x1, y1, w1, h1, x2, y2, w2, h2)) {
                // Check if they were colliding at last position
                bool wasColliding = rectanglesIntersect(lastX1, lastY1, w1, h1, lastX2, lastY2, w2, h2);
                
                if (!wasColliding) {
                    // They weren't colliding before, so move them back towards last positions
                    // Calculate movement vectors from last to current position
                    float moveX1 = x1 - lastX1;
                    float moveY1 = y1 - lastY1;
                    float moveX2 = x2 - lastX2;
                    float moveY2 = y2 - lastY2;
                    
                    // Calculate total movement magnitude for each entity
                    float moveMag1 = std::sqrt(moveX1 * moveX1 + moveY1 * moveY1);
                    float moveMag2 = std::sqrt(moveX2 * moveX2 + moveY2 * moveY2);
                    
                    // Calculate rectangle boundaries at current position
                    float rect1Left = x1;
                    float rect1Right = x1 + w1;
                    float rect1Top = y1;
                    float rect1Bottom = y1 + h1;
                    
                    float rect2Left = x2;
                    float rect2Right = x2 + w2;
                    float rect2Top = y2;
                    float rect2Bottom = y2 + h2;

                    // Calculate overlap
                    float overlapLeft = rect1Right - rect2Left;
                    float overlapRight = rect2Right - rect1Left;
                    float overlapTop = rect1Bottom - rect2Top;
                    float overlapBottom = rect2Bottom - rect1Top;

                    // Find the minimum overlap (the axis of least penetration)
                    float minOverlapX = std::min(overlapLeft, overlapRight);
                    float minOverlapY = std::min(overlapTop, overlapBottom);
                    
                    float totalMass = rigidbody1.Mass + rigidbody2.Mass;
                    if (totalMass > 0.0f) {
                        float massRatio1 = rigidbody2.Mass / totalMass;
                        float massRatio2 = rigidbody1.Mass / totalMass;
                        
                        // Prioritize vertical (Y-axis) collisions to prevent side clipping when falling
                        if (minOverlapY > 0.0f && (minOverlapY <= minOverlapX || minOverlapX <= 0.0f)) {
                            // Resolve collision on Y axis first
                            // Only move entities that have moved
                            float separation1 = 0.0f;
                            float separation2 = 0.0f;
                            
                            if (entity1Moved && entity2Moved) {
                                // Both moved, separate based on their movement direction
                                float moveRatio1 = moveMag1 / (moveMag1 + moveMag2);
                                float moveRatio2 = moveMag2 / (moveMag1 + moveMag2);
                                
                                if (overlapTop < overlapBottom) {
                                    // entity1 is on top, push it up
                                    separation1 = -minOverlapY * massRatio1 * moveRatio1;
                                    separation2 = minOverlapY * massRatio2 * moveRatio2;
                                } else {
                                    // entity1 is on bottom, push it down
                                    separation1 = minOverlapY * massRatio1 * moveRatio1;
                                    separation2 = -minOverlapY * massRatio2 * moveRatio2;
                                }
                            } else if (entity1Moved) {
                                // Only entity1 moved, push it back completely
                                if (overlapTop < overlapBottom) {
                                    separation1 = -minOverlapY;
                                    // entity2 stays stationary, no separation
                                } else {
                                    separation1 = minOverlapY;
                                    // entity2 stays stationary, no separation
                                }
                            } else if (entity2Moved) {
                                // Only entity2 moved, push it back completely
                                if (overlapTop < overlapBottom) {
                                    // entity1 stays stationary, no separation
                                    separation2 = minOverlapY;
                                } else {
                                    // entity1 stays stationary, no separation
                                    separation2 = -minOverlapY;
                                }
                            }
                            
                            // Only apply separation to entities that have moved
                            if (entity1Moved) {
                                transform1.position[1] += separation1;
                                // Update hitbox positions
                                rigidbody1.Hitbox.setPosition({transform1.position[0], transform1.position[1]});
                                rigidbody1.Hitbox.setSize({rigidbody1.base_size[0] * transform1.scale[0], rigidbody1.base_size[1] * transform1.scale[1]});
                                // Stop velocity only if collision is in the direction of movement
                                // If moving up (negative velocity) and being pushed down (positive separation), reset
                                // If moving down (positive velocity) and being pushed up (negative separation), reset
                                if ((rigidbody1.velocity[1] < 0.0f && separation1 > 0.0f) || 
                                    (rigidbody1.velocity[1] > 0.0f && separation1 < 0.0f)) {
                                    // If landing (moving down and being pushed up), reset jump
                                    if (rigidbody1.velocity[1] > 0.0f && separation1 < 0.0f) {
                                        jump_system.reset_jump(entity1);
                                    }
                                    rigidbody1.velocity[1] = 0.0f;
                                }
                            }
                            
                            if (entity2Moved) {
                                transform2.position[1] += separation2;
                                // Update hitbox positions
                                rigidbody2.Hitbox.setPosition({transform2.position[0], transform2.position[1]});
                                rigidbody2.Hitbox.setSize({rigidbody2.base_size[0] * transform2.scale[0], rigidbody2.base_size[1] * transform2.scale[1]});
                                // Stop velocity only if collision is in the direction of movement
                                // If moving up (negative velocity) and being pushed down (positive separation), reset
                                // If moving down (positive velocity) and being pushed up (negative separation), reset
                                if ((rigidbody2.velocity[1] < 0.0f && separation2 > 0.0f) || 
                                    (rigidbody2.velocity[1] > 0.0f && separation2 < 0.0f)) {
                                    // If landing (moving down and being pushed up), reset jump
                                    if (rigidbody2.velocity[1] > 0.0f && separation2 < 0.0f) {
                                        jump_system.reset_jump(entity2);
                                    }
                                    rigidbody2.velocity[1] = 0.0f;
                                }
                            }
                        } else if (minOverlapX > 0.0f) {
                            // Resolve collision on X axis (only if Y-axis wasn't resolved)
                            // Only move entities that have moved
                            float separation1 = 0.0f;
                            float separation2 = 0.0f;
                            
                            if (entity1Moved && entity2Moved) {
                                // Both moved, separate based on their movement direction
                                float moveRatio1 = moveMag1 / (moveMag1 + moveMag2);
                                float moveRatio2 = moveMag2 / (moveMag1 + moveMag2);
                                
                                if (overlapLeft < overlapRight) {
                                    // entity1 is on the left, push it left
                                    separation1 = -minOverlapX * massRatio1 * moveRatio1;
                                    separation2 = minOverlapX * massRatio2 * moveRatio2;
                                } else {
                                    // entity1 is on the right, push it right
                                    separation1 = minOverlapX * massRatio1 * moveRatio1;
                                    separation2 = -minOverlapX * massRatio2 * moveRatio2;
                                }
                            } else if (entity1Moved) {
                                // Only entity1 moved, push it back completely
                                if (overlapLeft < overlapRight) {
                                    separation1 = -minOverlapX;
                                    // entity2 stays stationary, no separation
                                } else {
                                    separation1 = minOverlapX;
                                    // entity2 stays stationary, no separation
                                }
                            } else if (entity2Moved) {
                                // Only entity2 moved, push it back completely
                                if (overlapLeft < overlapRight) {
                                    // entity1 stays stationary, no separation
                                    separation2 = minOverlapX;
                                } else {
                                    // entity1 stays stationary, no separation
                                    separation2 = -minOverlapX;
                                }
                            }
                            
                            // Only apply separation to entities that have moved
                            if (entity1Moved) {
                                transform1.position[0] += separation1;
                                // Update hitbox positions
                                rigidbody1.Hitbox.setPosition({transform1.position[0], transform1.position[1]});
                                rigidbody1.Hitbox.setSize({rigidbody1.base_size[0] * transform1.scale[0], rigidbody1.base_size[1] * transform1.scale[1]});
                                // Stop velocity in the collision direction
                                rigidbody1.velocity[0] = 0.0f;
                            }
                            
                            if (entity2Moved) {
                                transform2.position[0] += separation2;
                                // Update hitbox positions
                                rigidbody2.Hitbox.setPosition({transform2.position[0], transform2.position[1]});
                                rigidbody2.Hitbox.setSize({rigidbody2.base_size[0] * transform2.scale[0], rigidbody2.base_size[1] * transform2.scale[1]});
                                // Stop velocity in the collision direction
                                rigidbody2.velocity[0] = 0.0f;
                            }
                        }
                    }
                } else {
                    // They were already colliding at last position
                    // Only resolve if at least one entity has moved
                    if (entity1Moved || entity2Moved) {
                        // Calculate rectangle boundaries
                        float rect1Left = x1;
                        float rect1Right = x1 + w1;
                        float rect1Top = y1;
                        float rect1Bottom = y1 + h1;
                        
                        float rect2Left = x2;
                        float rect2Right = x2 + w2;
                        float rect2Top = y2;
                        float rect2Bottom = y2 + h2;

                        // Calculate overlap
                        float overlapLeft = rect1Right - rect2Left;
                        float overlapRight = rect2Right - rect1Left;
                        float overlapTop = rect1Bottom - rect2Top;
                        float overlapBottom = rect2Bottom - rect1Top;

                        // Find the minimum overlap (the axis of least penetration)
                        float minOverlapX = std::min(overlapLeft, overlapRight);
                        float minOverlapY = std::min(overlapTop, overlapBottom);
                        
                        float totalMass = rigidbody1.Mass + rigidbody2.Mass;
                        if (totalMass > 0.0f) {
                            float massRatio1 = rigidbody2.Mass / totalMass;
                            float massRatio2 = rigidbody1.Mass / totalMass;
                            
                            // Prioritize vertical (Y-axis) collisions to prevent side clipping when falling
                            if (minOverlapY > 0.0f && (minOverlapY <= minOverlapX || minOverlapX <= 0.0f)) {
                                // Resolve collision on Y axis first
                                float separation1 = 0.0f;
                                float separation2 = 0.0f;
                                
                                if (overlapTop < overlapBottom) {
                                    // entity1 is on top, push it up
                                    separation1 = -minOverlapY * massRatio1;
                                    separation2 = minOverlapY * massRatio2;
                                } else {
                                    // entity1 is on bottom, push it down
                                    separation1 = minOverlapY * massRatio1;
                                    separation2 = -minOverlapY * massRatio2;
                                }
                                
                                // Only apply separation to entities that have moved
                                if (entity1Moved) {
                                    transform1.position[1] += separation1;
                                    // Update hitbox positions and scales to match transforms
                                    rigidbody1.Hitbox.setPosition({transform1.position[0], transform1.position[1]});
                                    rigidbody1.Hitbox.setSize({rigidbody1.base_size[0] * transform1.scale[0], rigidbody1.base_size[1] * transform1.scale[1]});
                                    // Stop velocity only if collision is in the direction of movement
                                    // If moving up (negative velocity) and being pushed down (positive separation), reset
                                    // If moving down (positive velocity) and being pushed up (negative separation), reset
                                    if ((rigidbody1.velocity[1] < 0.0f && separation1 > 0.0f) || 
                                        (rigidbody1.velocity[1] > 0.0f && separation1 < 0.0f)) {
                                        // If landing (moving down and being pushed up), reset jump
                                        if (rigidbody1.velocity[1] > 0.0f && separation1 < 0.0f) {
                                            jump_system.reset_jump(entity1);
                                        }
                                        rigidbody1.velocity[1] = 0.0f;
                                    }
                                }
                                
                                if (entity2Moved) {
                                    transform2.position[1] += separation2;
                                    // Update hitbox positions and scales to match transforms
                                    rigidbody2.Hitbox.setPosition({transform2.position[0], transform2.position[1]});
                                    rigidbody2.Hitbox.setSize({rigidbody2.base_size[0] * transform2.scale[0], rigidbody2.base_size[1] * transform2.scale[1]});
                                    // Stop velocity only if collision is in the direction of movement
                                    // If moving up (negative velocity) and being pushed down (positive separation), reset
                                    // If moving down (positive velocity) and being pushed up (negative separation), reset
                                    if ((rigidbody2.velocity[1] < 0.0f && separation2 > 0.0f) || 
                                        (rigidbody2.velocity[1] > 0.0f && separation2 < 0.0f)) {
                                        // If landing (moving down and being pushed up), reset jump
                                        if (rigidbody2.velocity[1] > 0.0f && separation2 < 0.0f) {
                                            jump_system.reset_jump(entity2);
                                        }
                                        rigidbody2.velocity[1] = 0.0f;
                                    }
                                }
                            } else if (minOverlapX > 0.0f) {
                                // Resolve collision on X axis (only if Y-axis wasn't resolved)
                                float separation1 = 0.0f;
                                float separation2 = 0.0f;
                                
                                if (overlapLeft < overlapRight) {
                                    // entity1 is on the left, push it left
                                    separation1 = -minOverlapX * massRatio1;
                                    separation2 = minOverlapX * massRatio2;
                                } else {
                                    // entity1 is on the right, push it right
                                    separation1 = minOverlapX * massRatio1;
                                    separation2 = -minOverlapX * massRatio2;
                                }
                                
                                // Only apply separation to entities that have moved
                                if (entity1Moved) {
                                    transform1.position[0] += separation1;
                                    // Update hitbox positions and scales to match transforms
                                    rigidbody1.Hitbox.setPosition({transform1.position[0], transform1.position[1]});
                                    rigidbody1.Hitbox.setSize({rigidbody1.base_size[0] * transform1.scale[0], rigidbody1.base_size[1] * transform1.scale[1]});
                                    // Stop velocity in the collision direction
                                    rigidbody1.velocity[0] = 0.0f;
                                }
                                
                                if (entity2Moved) {
                                    transform2.position[0] += separation2;
                                    // Update hitbox positions and scales to match transforms
                                    rigidbody2.Hitbox.setPosition({transform2.position[0], transform2.position[1]});
                                    rigidbody2.Hitbox.setSize({rigidbody2.base_size[0] * transform2.scale[0], rigidbody2.base_size[1] * transform2.scale[1]});
                                    // Stop velocity in the collision direction
                                    rigidbody2.velocity[0] = 0.0f;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}