#pragma once

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices
{
    int graphicsFamily = -1; // Location of graphics queue family
    int presentationFamily = -1; // Location of presentation queue family

    // Check if queue families are valid
    bool isValid()
    {
        return graphicsFamily >= 0;
    }
};

