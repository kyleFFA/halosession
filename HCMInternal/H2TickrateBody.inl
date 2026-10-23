// Imported from Harc MasterTickrate.cpp; included inside H2MasterTickrate::Impl.
	using RecreateCollisionFn = uint32_t(__fastcall*)(uint16_t objectIndex);   // sub_180705B20 (self-no-ops if none)
	using GetObjectFn         = uintptr_t(__fastcall*)(uintptr_t headerEntry); // sub_1808D7000 (returns 0 if empty)
	static constexpr uint32_t  kRecreateCollisionRVA = 0x705B20;
	static constexpr uint32_t  kGetObjectRVA         = 0x8D7000;
	static constexpr uint8_t   kRecreateProbe[6] = { 0x40, 0x56, 0x48, 0x83, 0xEC, 0x20 };  // push rsi; sub rsp,20h
	static constexpr uint8_t   kGetObjectProbe[6] = { 0x8B, 0x51, 0x08, 0x48, 0x8B, 0x0D }; // mov edx,[rcx+8];mov rcx,[rip]
	static constexpr uint32_t  kObjectHeaderPtrRVA = 0x18B7398;  // -> qword_1818B7398 (object header table pointer)
	static constexpr uintptr_t kHeaderDataOffset   = 0x48;       // *(header+0x48) = offset from header to first entry
	static constexpr uintptr_t kHeaderCountOffset  = 0x40;       // *(header+0x40) = used/high-water object count
	static constexpr uint32_t  kHeaderStride       = 12;         // 12-byte header entries
	static constexpr uint32_t  kMaxObjects         = 2048;       // MAXIMUM_OBJECTS: array capacity + iteration clamp
	static constexpr uintptr_t kHavokComponentOffset = 0xB4;     // obj+0xB4 = collision body index (0xFFFFFFFF = none)

	static int rebuildAllCollisionSEH(uintptr_t base, GetObjectFn getObject, RecreateCollisionFn recreate)
	{
		__try
		{
			uintptr_t headerPtr = *(uintptr_t*)(base + kObjectHeaderPtrRVA); // qword_1818B7398 (object header table)
			if (!headerPtr || IsBadReadPtr((void*)headerPtr, 0x50)) return -1; // table not up / not readable
			uintptr_t dataRegion = headerPtr + *(uintptr_t*)(headerPtr + kHeaderDataOffset); // first header entry
			uint32_t count = *(uint32_t*)(headerPtr + kHeaderCountOffset);
			if (count > kMaxObjects) return -1; // an invalid table is not a successful partial rebuild

			int rebuilt = 0;
			for (uint32_t i = 0; i < count; ++i)
			{
				uintptr_t objData = getObject(dataRegion + (uintptr_t)kHeaderStride * i); // 0 for empty/deleted slots
				if (!objData) continue;
				if (*(uint32_t*)(objData + kHavokComponentOffset) == 0xFFFFFFFF) continue; // no collision body
				recreate((uint16_t)i);
				++rebuilt;
			}
			return rebuilt;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return -1;
		}
	}
