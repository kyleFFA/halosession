#pragma once
#include <cstdint>
#include <stdexcept>

// Verified for Steam Halo 3 1.3528.0.0. Reader must perform checked reads.
// Mapping layout agrees with engine TLS+0x110 helpers at RVAs EE0D4/EE100.
struct AutoCrouchIdentity
{
    uintptr_t pool{}, object{}, globals{}, mapping{};
    uint32_t player{}, unit{};
    uint8_t simulation{};
    bool operator==(const AutoCrouchIdentity&) const = default;
};
struct AutoCrouchLayout
{
    uintptr_t poolPointer, globalsOffset, mappingOffset, objectTable;
    uintptr_t ownerOffset, typeOffset, playbackOffset;
    uintptr_t allocationOffsets{}, globalsIndex{}, mappingIndex{};
    bool reach{};
    uintptr_t allocationBase{}; // ODST descriptors are relative to the game-state bank, not pool start.
};
inline constexpr AutoCrouchLayout autoCrouchHalo3{0x204FD18,0x47025C,0x4837E4,0x50749C,0x178,3,0x160};
// Engine allocation descriptors: index*0x30 gives a pool-relative offset.
inline constexpr AutoCrouchLayout autoCrouchODST{0x2094D48,0,0,0x50A364,0x170,3,0x160,0x2205A60,0x932510,0x932500,false,0x470000};
inline constexpr AutoCrouchLayout autoCrouchReach{0x24FB5F0,0,0,0x105F6C,0x1CC,4,0x1DA,0x27C5700,0xB886C0,0xB88390,true};
template<class Reader>
AutoCrouchIdentity readAutoCrouchIdentity(Reader& r, uintptr_t halo, const AutoCrouchLayout& layout = autoCrouchHalo3)
{
    auto ptr = [&](uintptr_t address) {
        auto value = r.template read<uintptr_t>(address);
        if (value < 0x10000 || value > 0x00007FFFFFFEFFFFULL)
            throw std::runtime_error("Player pointers unavailable; no movement applied");
        return value;
    };
    AutoCrouchIdentity s;
    s.pool = ptr(halo + layout.poolPointer);
    auto allocation = [&](uintptr_t indexRva, uintptr_t fixedOffset) {
        if (!layout.allocationOffsets) return s.pool + fixedOffset;
        const int32_t index = r.template read<int32_t>(halo + indexRva);
        if (index < 0 || index >= 1024) throw std::runtime_error("Game allocation unavailable");
        const auto offset = r.template read<uint32_t>(halo + layout.allocationOffsets + uintptr_t(index)*0x30);
        if (offset < 0x100 || offset > 0x10000000) throw std::runtime_error("Invalid game allocation");
        return s.pool + layout.allocationBase + offset;
    };
    s.globals = allocation(layout.globalsIndex, layout.globalsOffset);
    s.mapping = allocation(layout.mappingIndex, layout.mappingOffset);
    // Per-game engine simulation field; Reach's local/host cases are 1 and 5.

    s.simulation = r.template read<uint8_t>(s.globals + 0x11);
    if (s.simulation == 2 || s.simulation == 4)
        throw std::runtime_error("Auto-crouch is unavailable as a multiplayer client: Force Teleport cannot confirm host-authoritative movement");
    if ((s.simulation != 1 && s.simulation != 3 && s.simulation != 5) || (layout.reach && s.simulation == 3))
        throw std::runtime_error("Session authority unknown; no movement applied");
    if (r.template read<uint8_t>(s.globals + layout.playbackOffset) != 0)
        throw std::runtime_error("Auto-crouch is unavailable during playback");
    const auto mapping = s.mapping;
    if (r.template read<int16_t>(mapping) != 1)
        throw std::runtime_error("Exactly one local input user is required");
    int selected = -1;
    for (int i = 0; i < 4; ++i)
    {
        const auto player = r.template read<uint32_t>(mapping + 4 + 4*i);
        if (player == 0xFFFFFFFF) continue;
        if (selected != -1) throw std::runtime_error("Local player mapping is ambiguous");
        selected = i;
        s.player = player;
        s.unit = r.template read<uint32_t>(mapping + 0x14 + 4*i);
    }
    if (selected != 0) throw std::runtime_error("Local input user/camera mapping unavailable; no slot-zero fallback");
    const auto playerIndex = s.player & 0xFFFF;
    if (playerIndex >= 16 || r.template read<int32_t>(mapping + 0x74 + 4*playerIndex) != selected)
        throw std::runtime_error("Local player mapping does not round-trip");
    if (s.unit == 0xFFFFFFFF || (s.unit & 0xFFFF) >= 2048)
        throw std::runtime_error("Local player's biped unavailable");
    const auto header = s.pool + layout.objectTable + (s.unit & 0xFFFF)*0x18;
    if (r.template read<uint16_t>(header) != (s.unit >> 16) || r.template read<uint8_t>(header + layout.typeOffset) != 0)
        throw std::runtime_error("Local biped identity is stale");
    s.object = ptr(header + 0x10);
    // Engine unit->player owner lookup at RVA EE166 uses +0x178.
    if (r.template read<uint32_t>(s.object + layout.ownerOffset) != s.player)
        throw std::runtime_error("Biped is not owned by the local player");
    if (ptr(halo + layout.poolPointer) != s.pool
        || allocation(layout.globalsIndex, layout.globalsOffset) != s.globals
        || allocation(layout.mappingIndex, layout.mappingOffset) != s.mapping)
        throw std::runtime_error("Session changed during player resolution");
    return s;
}
