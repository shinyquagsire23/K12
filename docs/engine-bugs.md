# Original engine bugs

Fixes live in `src/` behind `#ifdef K2_FIX_ENGINE_BUGS`, which only the host builds define
(`platforms/`, CMake option `K2P_FIX_ENGINE_BUGS`, default ON). The matching build never defines
it, so the transcription itself stays faithful. Platform/ABI adaptations use `K2_HOST_BUILD`
instead, and are not engine bugs. An entry with a fix says **Fixed under `K2_FIX_ENGINE_BUGS`**.

**Frame-rate fixes use 60 fps as the canonical rate** (user decision, 2026-10-20: the PC releases'
maximum; K1 Android's 40 fps is not the reference). A per-frame quantity that a fix rescales is
rescaled to its size at 60 fps, so nothing changes at the default 60 fps lock.

## `ExecuteCommandHasLineOfSight` — the "no area" arm pushes its answer and then calls through a NULL `this`

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandHasLineOfSight` @0x584630. When the script
  owner resolves to no area — its `AsSWSArea()` is null and so are the creature / door / placeable
  `GetArea()` fallbacks — the handler pushes `FALSE` and then, instead of returning, **falls
  through** into the `CSWSArea::ClearLineOfSight(...)` call site with the area pointer explicitly
  zeroed (`mov x0, xzr` at 0x5895a0).
- **Impact:** a double push onto the VM stack followed by a null dereference inside
  `ClearLineOfSight`. Reachable from any script whose `OBJECT_SELF` is not in an area.
- **Confirmation:** the macOS build does exactly the same thing — `xorl %r14d,%r14d` at
  0x10031ec33 immediately before the `movq %r14,%rdi` that sets up the call. Two different
  compilers on two targets do not independently invent a fallthrough, so the missing `return` is
  in the SOURCE. Transcribed verbatim (v945) with the `return` left out, so the bug is visible.

## `ExecuteCommandGetJournalQuestExperience` — leaks a whole GFF reader when global.jrl fails to load

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandGetJournalQuestExperience` @0x57b3f0.
  It `new`s a `CResGFF(0x808, "JRL ", CResRef("Global"))` on EVERY call and deletes it through the
  virtual destructor at the end — but only on the path where `m_nValid != 0`. The invalid arm
  (0x57b548) jumps straight to the push.
- **Impact:** every `GetJournalQuestExperience()` call against a missing/corrupt global.jrl leaks
  the reader and its parsed struct/field tables. Also note the non-bug cost: the file is re-read
  from disk on every single call, with a linear scan of the "Categories" list.

## `ExecuteCommandSetItemStackSize` — the feedback message object is never freed

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandSetItemStackSize` @0x58a0e8. A
  `CSWCCMessageData` is `new`ed for the "you gained/lost N items" report; the only `operator
  delete` in the function is the constructor's landing pad, so no normal path frees it.
- **Impact:** 0x48 bytes leaked per real stack-size change on a party-carried item.

## `ExecuteCommandCreateItemOnFloor` — the appear-animation argument is popped and discarded

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandCreateItemOnFloor` @0x568cac. nwscript's
  third parameter `int bUseAppearAnimation` is popped into a stack slot (`sp+0xc`) that nothing
  ever reads; the item is placed with `AddToArea(..., 1)` regardless.
- **Impact:** `CreateItemOnFloor(sTemplate, lLoc, TRUE)` behaves exactly like the FALSE form. The
  slot is still seeded to 0 before the pop, so it is a real parameter that lost its consumer, not
  a mis-registered id.

## `SWMG_GetTargettingGunBank*` — every one of the four getters is DEAD: the handler switches on the SETTER's command ids

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandSWMG_GetTargettingGunBankFloatInfo`
  @0x64dc30. `InitializeCommands` @0x55ca28 registers it for `m_paCommands[657..660]`
  (`SWMG_GetTargettingGunBankHorizontalSpread` / `…VerticalSpread` / `…SensingRadius` /
  `…Inaccuracy`), but the body's dispatch is `sub w8, w19, #0x295` (0x295 = **661**) followed by
  `cmp w8, #3`. 661..664 are the ids of the *setter*, `ExecuteCommandSWMG_SetTargettingGunBankFloatInfo`
  @0x64dd54 — whose own `sub w8, w19, #0x295` is correct.
- **Impact:** for every id that can actually reach the getter, `nCommand - 661` is negative, the
  `b.hi` takes the default, and the pre-seeded `0.0f` is pushed. All four auto-aim getters return
  0.0 unconditionally, even for a fully-configured targetting gun bank. The setters work.
- **Confirmation:** the two functions' jump tables (0x9cdbfe for the getter, 0x9cdc02 for the
  setter) map the SAME four offsets onto the same four `CSWMGTargettingParameters` fields
  (+0x78 horizontal spread, +0x80 vertical spread, +0x7c sensing radius, +0x84 inaccuracy), so the
  getter's table was written for 661..664 too — the registration, not the table, is the odd one out.
  Transcribed verbatim in v940 (`case 661: … case 664:` in the getter), which is what makes the
  bug visible in the source rather than silently "fixed" into 657..660.
- ⭐ **The generalisable check:** when a one-handler-many-ids command switches on `nCommand`, diff
  the switch's id set against the `m_paCommands[...]` rows that name it. A copy-pasted handler pair
  is exactly where the two drift apart, and no tool sees it — the function byte-matches either way.

## `ExecuteCommandGiveXPToCreature` — the creature operand does not decide who gets the XP

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandGiveXPToCreature` @0x58450c. The popped
  creature id is used only to (a) reach the resolved creature's `m_bInParty` flag, which gates the
  on-screen XP ticker, and (b) nothing else. The award itself is
  `GetPartyTable()->DistributeExperience(nXP, 1)` — a whole-party call that never sees the operand.
- **Impact:** `GiveXPToCreature(oNonPartyNPC, 500)` still pays the player's party 500 XP; the only
  observable difference from passing a party member is that the HUD does not animate. A script that
  tries to award XP to a specific companion cannot.
- **Both builds agree** (macOS oracle @0x10031e9c8: the same `DistributeExperience(-0x34(%rbp), 1)`
  tail after the `m_bInParty`-gated `UpdateStatus`).

## `ExecuteCommandDelayCommand` / `ExecuteCommandAssignCommand` / `ExecuteCommandSignalEvent` — inconsistent ownership of the queued blob

- **Where:** all three pop an engine-owned heap blob (a script situation for the first two, a script
  event for the third) and hand it to `CServerAIMaster::AddEventDeltaTime`, which takes ownership.
- **Impact:** `AssignCommand` @0x5675dc and `DelayCommand` @0x569a70 both free the situation with a
  bare `operator delete` when the target object does not resolve — but `SignalEvent` @0x58c2c8 does
  **not**: its unresolved-target path returns 0 and simply drops the pointer, LEAKING one script
  event per failed `SignalEvent`. Scripts that signal at objects destroyed earlier in the same
  frame leak steadily.
- Also worth noting: `DelayCommand` refuses outright (and frees) whenever `m_nScriptRunArg != 1`,
  i.e. outside a real action script, while `AssignCommand` has no such gate.

## `CSWSCreature::AIActionExamineMine` — the skill-check DC is read UNINITIALIZED on one path

- **Where:** `AIActionExamineMine` @0x53cca0's object-type dispatch. Three arms compute the disarm
  DC into `w24` (trigger @0x53d174, door @0x53cf84, placeable @0x53d1c0), each as
  `max(1, GetDisarmDC() - 7)`. The FOURTH edge — the examined object is none of trigger, door or
  placeable (0x53d1e8) — sets only `w23 = 0` (the "own mine" flag) and `[x29,#-0x98] = 0` (the
  success flag) and falls straight through to the message build, which then ships `w24` verbatim as
  `CSWCCMessageData::SetInteger(3, nDC)` @0x53d27c. Nothing on that path ever writes `w24`.
- **Impact:** the client's mine-examination panel is handed whatever the register happens to hold —
  in practice a leftover from the roll block or from `GetSkillRank`'s frame. It is display-only
  (`bSuccess`, which drives the actual outcome, IS correctly 0 on that edge), so the visible symptom
  is a garbage "DC" number in the examine GUI rather than a wrong game result. Reachable only if a
  mine action is ever queued against an object that is not one of the three trap-bearing types.
- **Both builds agree**: the macOS oracle leaves `%ebx` equally undefined on its `0x10043a49e` edge
  (`movl $0x0, -0x70(%rbp)` and nothing else) before `SetInteger(3, %ebx)` @0x10043a4ff.
  Transcribed verbatim — the `int nDC;` declaration is deliberately left uninitialized. Do NOT
  "fix" it by adding `= 0`; that costs bytes and changes the CFG.
- ⭐ **The generalisable check:** when a value is assigned in every arm of an if/else-if chain but
  the chain has a FINAL implicit `else`, check whether that else path reaches a consumer of the
  value. A shared `mov` block feeding N arms is easy to mistake for an initializer — read which
  edges actually reach it.
- ⭐⭐ **SECOND SIGHTING, and it is WORSE in the sibling (v669): `CSWSCreature::AIActionFlagMine`
  @0x53aca8 has the same bug with TWO reaching edges instead of one.** Besides the same
  none-of-the-three-types edge (0x53b254, which sets only `w27 = 0`), the OWN-MINE path also
  reaches the message build without ever writing `w24`: at 0x53add4 the trigger-owned arm does
  `mov w27,#1 / str w27,[x0,#0x454] / b 0x53b288`, so `SetInteger(3, nDC)` @0x53b2e4 ships the low
  word of the `AsSWSTrigger()` POINTER. Flagging your own mine therefore always displays a garbage
  DC. Both builds agree (the macOS oracle's `0x100437cb9` edge does `movl %eax, -0x78(%rbp)` with
  `%eax` holding the same pointer). Transcribed verbatim; the `int nDC;` declaration is deliberately
  uninitialized in that row too.
- ⭐ **What the pair adds to the check above:** the uninitialized edge need not be the *implicit*
  else. Here one of the reaching edges is an explicit, ordinary-looking `if` arm that simply has no
  reason to compute a DC — it skips the roll entirely. **Enumerate every predecessor of the
  consumer block, not just the fall-through one.**

## `CSWSCreature::AIActionDropItem` — the container check dereferences `pPlayer` unguarded

- **Where:** `AIActionDropItem` @0x54e810, the "close the container's inventory before taking the
  item out of it" guard. It loads `pPlayer->m_pUnk88` with `ldr x8,[x22,#0x88]` (macOS
  `movq 0x88(%r15),%rax` @0x10044e005) and `x22`/`%r15` is the `CServerExoAppInternal::
  GetClientObjectByObjectId(m_objectId)` result — which every OTHER use in the same function
  null-tests first (`if (pMessage != 0 && pPlayer != 0)` at all three message sites).
- **Impact:** a latent null dereference whenever a creature with no controlling player drops an
  item that currently sits inside a container it owns. Reachable in principle for NPC party members
  and script-driven drops; the two guarded paths around it show the author knew the pointer was
  optional.
- **Both builds agree** on the unguarded load, so it is source. Transcribed verbatim — do NOT add
  the missing null test.
- ⭐ **The generalisable check:** when a function null-tests a pointer at some uses and not others,
  the UNGUARDED ones are the finding. Grep the row's disasm for every use of the register holding
  the optional pointer, not just the ones near the branches you already transcribed.

## `CSWSCreature::AIActionPickUpItem` — the pick-up notify flag is read from the WRONG PACK SLOT

- **Where:** `AIActionPickUpItem` @0x54e26c reads the action node's notify flag with
  `ldrb w24,[x20,#0x50]` (macOS x86_64 `movb 0x50(%rbx),%al` @0x10044d299) — pack slot **3**.
  Its writers put that flag in slot **2** (`+0x48`): `CSWSCreature::AddPickUpItemActions`
  (macOS @0x100380646) passes it as `CSWSObject::AddAction`'s THIRD `(tag, ptr)` pair
  (`type1=3, &oidItem` / `type2=3, &oidDest` / `type3=1, &bNotify`), and this row's own re-queue
  repeats that argument shape verbatim. `AddActionNodeParameter` maps index *i* to `+0x38 + i*8`,
  so slot 2 is `+0x48` and slot 3 is `+0x50`.
- **Impact:** the flag the caller asked for never reaches the AI. The reader picks up whatever slot
  3 holds — 0 for both known queuers — so `CSWSCreature::AcquireItem`'s fourth argument is always
  false and the "you picked something up" GUI notification never fires from this path.
- **Both builds agree** on the `+0x50` read and on the writers' argument positions, so it is
  source, not a transcription artifact. Transcribed verbatim — do NOT re-point the reader at
  `+0x48`. Modelled as `CSWSObjectActionNode::m_bPickUpNotify` in the `+0x50` union.
- ⭐ **The generalisable check:** whenever a tagged-pack reader and its writer are both available,
  count the writer's `(tag, ptr)` argument positions against the reader's load offsets. An
  off-by-one there is invisible to `match.py`, to `constaudit.py` and to every other tool.

## `CSWSCreature::AIActionPickUpItem` — the pick-up animation is chosen from a zeroed local

- **Where:** the same function's "start the reach animation" branch. The animation id is computed
  as `0x274b + (vDest.z > 0.0f)` (macOS `movss -0x50(%rbp),%xmm0` / `ucomiss` / `seta` /
  `addl $0x274b` @0x10044d7d1..0x10044d7e3), but `vDest` is the function's zero-initialised
  destination `Vector` and nothing has written it on this path — the only writes are in the
  far/walk-to branch, which returns before reaching here. The author plainly meant the ITEM's
  height (`pItem->m_position.z`).
- **Impact:** the "reach up high" pickup animation (0x274c) is unreachable; every ground pickup
  plays the low variant 0x274b.
- **The builds DIVERGE in codegen but not in source:** arm64 constant-folds `0.0f > 0.0f` and emits
  a bare `mov w10,#0x274b`, while the macOS build keeps the compare. Transcribed verbatim — the
  zeroed-local spelling is LOAD-BEARING, since reading the item's real z would be both wrong and
  longer.

## `CSWSCreature::RunUnequip` — the partner-weapon message dereferences a NULL item

- **Where:** `RunUnequip` @0x62349c, the verdict-2/3 (weapon-set) arm. The partner slot is fetched
  with `GetItemInSlot(nVerdict == 3 ? 0x80000 : 0x20)` and the unequip/re-equip pair that follows is
  correctly guarded by a null test (arm64 `cbz x0,#0x623800` at 0x6237c8). But the very next
  statement — the first `SendServerToPlayerInventory_Unequip` — reads `pPartner->m_objectId`
  **outside** that guard (`ldr x2,[x27,#0x20]` at 0x623848, with `x27` still whatever
  `GetItemInSlot` returned).
- **Impact:** unequipping one half of a wielded weapon set while the other hand is empty faults on
  a NULL dereference. It survives in practice because reaching this arm at all means the item was
  matched against an occupied primary or alternate weapon slot, so the partner is usually present.
- **Both builds agree** (arm64 @0x623848; macOS x86_64 `movq 0x20(%r13),%rdx` @0x10038050b, past
  the `je 0x1003804bd` at 0x100380489), so it is source, not a transcription artifact.
  Transcribed verbatim — do not "fix" it.

## `CSWSCreature::RunUnequip` — a completed unequip reports FAILURE when there is no client

- **Where:** the same function's message tail, on both the verdict-2/3 and verdict-1 arms. The
  `return 1` sits **inside** `if (pPlayer != 0 && pMessage != 0)`; when either is null the arm falls
  THROUGH into the shared failure tail at 0x623938, which sets `w0 = 0` (and then skips its own
  `SendServerToPlayerInventory_UnequipCancel` because the same two null tests fail again).
- **Impact:** the item has already been unequipped, added to the repository (or dropped into the
  area) and re-possessed by the time this is reached, so the caller is told the operation failed
  after it fully succeeded. Callers such as `AIActionUnequipItem` @0x550068 gate on `== 1`.
- **Both builds agree** (arm64 `cbz x24,#0x623938` / `cbz x23,#0x623938` at 0x623838; x86
  `je 0x100380615` at 0x1003804f3 and 0x1003804fe). Transcribed verbatim.

## `CSWSCreature::HandleSubAreaStateChanges` — a subarea ENTER event is queued with an UNINITIALISED event type

- **Where:** `HandleSubAreaStateChanges` @0x547db0, the enter pass. The event object is allocated
  and constructed up front (`new CScriptEvent`), then one of three arms assigns `m_eventType`:
  `0x0c` for a plain subarea, `0x1e` for a door / area-transition trigger reached by the party
  leader, or the whole event is deleted for a non-leader party member. But when the subarea IS a
  door / area-transition and `m_Unk0x17b4 == 0` (the creature is not in the party at all), BOTH
  guards branch straight past every assignment to the `SetObjectID` at 0x548158 — see the `cbz w8`
  at 0x5480ec and again at 0x54812c.
- **Why it is a bug and not a default:** `CScriptEvent::CScriptEvent` @0x6b4ebc zeroes only
  +0x08..+0x47 and **leaves +0x00 (`m_eventType`) alone**, so the event reaches
  `CServerAIMaster::AddEventDeltaTime` carrying whatever was in the freshly-`operator new`'d block.
- **Impact:** a non-party creature walking into a door or area-transition subarea queues a script
  event of an arbitrary type against that subarea. Harmless most of the time because the allocator
  usually hands back recently-freed CScriptEvent storage whose first two bytes are a plausible
  type, which is presumably why it survived shipping.
- **Both builds agree** (arm64 @0x547db0, macOS x86_64 @0x10044613c), so it is source, not a
  transcription artifact. Transcribed verbatim — do not "fix" it.

## `CSWSCreature::SignalMeleeDamage` / `SignalRangedDamage` — the ATTACK INDEX is passed as a DELTA TIME

- **Where:** `SignalMeleeDamage` @0x674e90 and `SignalRangedDamage` @0x674898. Both queue an
  `EVENT_ON_MELEE_ATTACKED` through
  `CServerAIMaster::AddEventDeltaTime(unsigned int nDay, unsigned int nDeltaMs, ...)`, and both
  pass the **attack index** as `nDeltaMs` — the melee one passes its loop counter `i`
  (`mov w2, w27`), the ranged one passes `m_pCombatRound->m_currentAttack` (`mov w2, w20`).
  Every OTHER `AddEventDeltaTime` in the same two functions passes a real time: the accumulated
  animation delay in the melee case, `m_Unk0x18` (the projectile flight time in ms) in the ranged
  case. Both builds agree, so it is source, not a transcription artifact.
- **Impact:** cosmetic/timing. The Nth melee attack's on-attacked AI signal fires N **milliseconds**
  into the round instead of at that attack's animation time, so the defender's reaction is
  effectively immediate and desynchronised from the swing that caused it. In the ranged case the
  index is 0..4, so the signal is essentially always "now".
- **Almost certainly** a copy-paste of the argument slot that should have held the same delay the
  neighbouring calls use.


## `CSWSCreature::RemoveComponent` / `RemoveChemical` — the remaining count is stored into GOLD

- **Where:** `CSWSCreature::RemoveComponent` (0x63b718) and `CSWSCreature::RemoveChemical`
  (0x63ba34). Both read and clamp the right field (party table +0x1b4/+0x1b8, or the creature's
  own m_nComponent +0x16d0 / m_nChemical +0x16d4) and compute the delta correctly — but the
  store goes through `SetGold`: the arm64 store targets are the GOLD slots (+0x1b0 / +0x16b0),
  and SetGold's `>= 10000 && m_bPlayerMadeCharacter -> Achievements::UnlockAchievement(0x21)`
  block is inlined into both bodies (Ghidra agrees). i.e. spending components/chemicals
  overwrites the party's gold with the remaining component/chemical count (and can award the
  "rich" achievement). The sibling `RemoveGold` (0x63a348) shares the same skeleton minus the
  wrong store target — almost certainly a copy-paste defect in the original source.
- **Impact:** player-visible: any script/feedback path that removes components or chemicals
  clobbers the gold total.
- **Reproduction:** the two `SetGold(nNew)` calls are marked `// sic:` in
  `src/game/servercore/CSWSCreature/CSWSCreature.cpp`.

Verified-against-disasm defects in the shipped KOTOR II Android binary that our byte-exact
source must faithfully reproduce. Each reproduction site in `src/` is marked with a `// sic:`
comment. Do NOT "fix" these — matching them exactly is the point.

## `CSWSExpression::TestString` — the graph walk dereferences the successor before its null test

- **Where:** `CSWSExpression::TestString(const CExoString&, int)` (0x591de4), the advance shared by
  every forward-moving arm of the node-type switch. The order is `ldr x25,[x8]` (0x592290) →
  `str wzr,[x25]` (0x592294) → … → `cbnz x25` (0x5922a4): the successor's `m_nVisits` is ZEROED
  before the loop's own `while (pNode != NULL)` test runs. The macOS build has the identical order
  (`movq (%r13),%r13` / `movl $0x0,(%r13)` / `testq %r13,%r13` at 0x10034ed7e), so it is the source
  and not an arm64 scheduling artifact — a store cannot be hoisted across the branch that guards it.
- **Impact:** a null write (offset 0) on any compiled graph whose last node is not a type-9 accept
  node. Unreachable in practice: `ParseString` always appends `NewNode(9)`, and the type-9 arm
  either accepts or backtracks — it never advances. Any future pattern source that produced a
  graph ending otherwise would crash here rather than fail the match.
- **Reproduction:** the `pNode->m_nVisits = 0;` statements after each `pNode = pNode->m_pNext;` /
  `pNode = pNode->m_pAlternate;` in
  `src/game/servercore/CSWSExpression/CSWSExpression.cpp`, flagged in the function's header comment.

## `CSWSEncounter::AddToActivateAreaList` — grow-copy reads one element past the old array

- **Where:** `CSWSEncounter::AddToActivateAreaList(unsigned long long)` (0x59c2b8). When the
  activation-area object-id list is full (`count >= capacity`) it allocates a `capacity+16`
  array and copies the old contents — but the copy trip count is `count + 1` (disasm: `add
  w8,w22,#1; mov w9,w8` feeding the copy loop), not `count`. Since the list was full, the old
  array holds exactly `count` valid elements, so the loop reads `pArr[count]` — 8 bytes past the
  end of the old heap allocation.
- **Impact:** harmless in practice: the over-read value lands in `pNew[count]`, which is
  immediately overwritten by `objectId` on the next line, so no incorrect data survives. But it
  is a genuine 8-byte heap over-read in the shipped build (would trip ASan / a guard page).
- **Reproduction:** the grow branch copies `newCount = (int)count + 1` elements — see the
  `// sic:` comment in `src/CSWSEncounter/CSWSEncounter.cpp`.
- **Fixed under `K2_FIX_ENGINE_BUGS`** (warnings ledger, 2026-10-22): the grow copies `count` elements.

## `CServerExoAppInternal::m_pPendingAuthIds` — allocated with `new[]`, freed with plain `delete`

- **Where:** ctor `CServerExoAppInternal::CServerExoAppInternal()` (0x5fcb44) allocates the
  16-slot pending-auth-id array via `operator new[](unsigned long)` (PLT symbol `_Znam`,
  resolved at 0x2f2ee0 — confirmed via `objdump -d -j .plt`, distinct from the plain
  `operator new(unsigned long)` / `_Znwm` used everywhere else in the same function for
  single-object allocations). The destructor `~CServerExoAppInternal()` (0x5fceb4) frees it
  with plain `operator delete(void*)` (PLT symbol `_ZdlPv`), NOT `operator delete[]` (`_ZdaPv`,
  which the same dtor correctly uses for the two `CExoArrayList<T>` id lists a few lines
  earlier via their EH-landing-pad dtor calls).
- **Impact:** technically UB per the C++ standard (mismatched array-new/scalar-delete), but
  harmless in practice for a POD `int[16]` under bionic's allocator (no destructor to skip, no
  array-cookie-dependent size tracking observed in the disasm on either side).
- **Reproduction:** `m_pPendingAuthIds = new int[16];` in the ctor, `delete m_pPendingAuthIds;
  // sic: orig uses scalar delete, not delete[], on an array-new'd pointer` in the dtor.

## `CFactionManager::GetFactionIdByName` — inverted match/mismatch branches + a duplicated check

- **Where:** `CFactionManager::GetFactionIdByName(const CExoString&)` (0x6954c8). Six sequential
  `if (sName.CompareNoCase(LITERAL) != 0) return CONST;` guards (returning 1/2/4/3/5/6 for
  hostile_1/friendly_1/friendly_2/hostile_2/neutral/hostile_2-again respectively) precede the
  "real" logic (a linear scan of `m_paFactions` comparing each entry's name to `sName`, returning
  its array index). Because each guard returns early on MISMATCH (`!= 0`) and only continues
  checking further literals on MATCH (`== 0`), calling this with an actual literal name almost
  never reaches the array scan at all — e.g. `GetFactionIdByName("hostile_1")` matches the first
  guard (continues), then immediately mismatches `"friendly_1"` and returns `2`, NOT any id
  related to "hostile_1". The 6th guard also re-checks `"hostile_2"` a second time (identical
  string to guard #4) instead of a plausible intended 6th distinct literal (e.g. `"insane"`) —
  a second copy-paste bug layered on the first.
- **Impact:** for any of the 6 known literal faction-name strings, the function returns one of
  the fixed constants 1–6 based on which OTHER literal it first fails to match, not a value
  connected to the name that was actually passed in. Only names that don't match ANY of the 6
  literals ever reach the (correct) array-scan fallback.
- **Reproduction:** see the `// sic:` comments in `src/CFactionManager/CFactionManager.cpp`.

## `CExoStringList::Add` — the `m_bSorted` branch sense is inverted

- **Where:** `CExoStringList::Add(CExoString*)` (0x7a11f8). The very first check is
  `if (m_bSorted == 0) { AddSorted(pString); return; }` — i.e. when the list is NOT flagged
  sorted, `Add()` forwards to `AddSorted()`, which unconditionally performs a real
  sort-order-preserving binary insert (it never itself reads `m_bSorted`). When the list IS
  flagged sorted, `Add()` instead falls through to its own inline logic, which does a linear
  duplicate scan (gated by `m_bDuplicate`) and then a plain unsorted append at the current
  `m_nCount` (the "shift to open a slot" logic it shares with `Introduce()`/`Insert()` is always
  a no-op here since the insert position is always the current count).
- **Impact:** calling `Add()` on a list constructed with `bSorted = 0` silently sorts it via
  `AddSorted()`'s binary insert; calling it on a list constructed with `bSorted = 1` instead
  appends unsorted, which will violate sort order on any list with more than one distinct value.
  Both directions are the opposite of what the flag name and the class's own `GetSorted()`
  getter would suggest.
- **Reproduction:** see the `// sic:` comment above `CExoStringList::Add` in
  `src/CExoStringList/CExoStringList.cpp`.

## `CExoIniInternal::ReadIniEntry` — the fgets scratch buffer is never freed

- **Where:** `CExoIniInternal::ReadIniEntry(CExoString&, const CExoString&, const CExoString&,
  const CExoString&)` (0x79ca80). The generic (non-hardcoded-key) path allocates a 512-byte
  scratch buffer via `operator new[](0x200)` (`new char[0x200]`), used as the `fgets` line
  buffer while parsing the ini file into the in-memory section/entry tree AND, afterward, as a
  reusable scratch buffer for stripping brackets off each candidate section-header line while
  searching for the requested section. No exit path in the function — success, "file not found",
  "section not found", "key not found", or the file-already-cached fast path — ever calls
  `operator delete[]` on it. Confirmed by exhaustively resolving every `bl` target in the
  original disasm (`tools/bltally.py`): `_Znam` (`operator new[]`) appears once, `_ZdaPv`
  (`operator delete[]`) appears zero times, anywhere in the function.
- **Impact:** every call to `ReadIniEntry()` that reaches the generic parse/search path (i.e.
  every key other than the 3 hardcoded ones) leaks 512 bytes. Since this is called repeatedly
  during startup/options handling, it's a real (if small) per-call leak in the shipped binary.
- **Reproduction:** see the `// sic:` comment above the `new char[0x200]` allocation in
  `src/CExoIni/CExoIniInternal.cpp`.

## `CExoResMan::AddKeyTable` — a formatted warning message is built and then never used

- **Where:** `CExoResMan::AddKeyTable(const CExoString&, unsigned int, unsigned int)`
  (0x7c1fa8). When adding a resource-image-pack table (`sourceType==4`) whose name matches one
  already in the list, the "already exists, rebuild it" path default-constructs a stack
  `CExoString`, and — ONLY for that source type — calls `CExoString::Format()` on it with the
  literal `"Table being rebuilt, this RIM is being leaked: %s"` and the offending path. The
  formatted string is never passed to any logging/warning call, never printed, never stored
  anywhere — the very next thing that happens is `RebuildTable()`, then the temporary is
  destructed. Confirmed by exhaustively resolving every `bl` target in the original disasm: the
  `CExoString::Format` call has no `CExoDebug::Warning`/output call anywhere between it and the
  temporary's destructor.
- **Impact:** none observable (the message describes a real leak — an old RIM's resources being
  orphaned by the rebuild — but the warning meant to surface it is silently swallowed). A minor,
  harmless instance of dead engine code, kept for byte-fidelity.
- **Reproduction:** see the `// sic:` comment above the `Format()` call in
  `src/CExoResMan/CExoResMan.cpp`.

## `CExoResMan::GetEncapsulatedFileDescription` — leaks the `CExoEncapsulatedFile` on open failure

- **Where:** `CExoResMan::GetEncapsulatedFileDescription(const CExoString&)` (0x7c41b4). Allocates
  a `new CExoEncapsulatedFile()`, sets its filename, then calls its virtual `OpenFile()`. On
  success it copies the description, calls `CloseFile()`, and deletes the object (via the vtable
  slot-1 deleting destructor) before returning. On FAILURE (`OpenFile()` returns false), the
  function returns an empty `CExoLocString` immediately — the allocated `CExoEncapsulatedFile` is
  never `CloseFile()`'d or deleted. Confirmed by exhaustively tracing every `blr`/`bl` in the
  original disasm: the failure path (`0x7c4250`) contains exactly 2 instructions (construct the
  empty return value) with no call to the object's CloseFile or deleting-destructor vtable slots.
- **Impact:** a real per-call heap leak of `sizeof(CExoEncapsulatedFile)` (0x70 bytes) plus
  whatever `OpenFile()` itself allocated before failing, triggered whenever a query for an
  encapsulated (ERF/RIM) file's description targets a file that fails to open.
- **Reproduction:** see the `// sic:`-documented method comment in `src/CExoResMan/CExoResMan.h`
  and the EFFECTIVE note above `GetEncapsulatedFileDescription` in `src/CExoResMan/CExoResMan.cpp`.

## `CExoKeyTable::CExoKeyTable()` — recomputes the shared CRC32 table on every construction

- **Where:** `CExoKeyTable::CExoKeyTable()` (0x7b2d54). The class's 256-entry CRC32 lookup table
  (`m_nCRC32`, the standard reversed-poly zlib generator, poly `0xEDB88320`) is a `static` member
  — ONE shared table used by every instance's `Hash()`/`FindKey()`/`AddKey()` — yet the ctor
  unconditionally regenerates the whole table (an autovectorized NEON loop) every time a
  `CExoKeyTable` is constructed, with no `__cxa_guard_acquire`-style once-guard anywhere in the
  disasm. Confirmed by disassembling the ctor directly: the table-generation loop runs
  unconditionally right after the field-init block, identically on every call.
- **Impact:** wasted CPU on every `CExoKeyTable` construction (`CExoResMan` owns several — one per
  fixed KEY file, one per resource-image pack, etc.) recomputing an identical, purely-constant
  256-entry table each time. Not a correctness bug (the table's contents are deterministic and
  always end up correct), just redundant work.
- **Reproduction:** see the ctor comment in `src/CExoKeyTable/CExoKeyTable.cpp`.

## `CExoRawInputInternal::CExoRawInputInternal` — unconditional `CheckControllerType` call can
## null-deref after a failed joystick `SetCooperativeLevel`

- **Where:** `CExoRawInputInternal::CExoRawInputInternal(HINSTANCE__*, HWND__*)` (0x7ae510), the
  per-joystick setup loop. For each enumerated joystick: `SetDataFormat` failing Releases the
  device and nulls `m_paJoystickDevices[i]`, guarded by an immediately-following
  `if (m_paJoystickDevices[i] == 0) continue;` — safe. `SetCooperativeLevel` failing ALSO Releases
  the device and nulls the same slot, but there is NO equivalent guard before the very next
  statement, `CheckControllerType(i)`, which is called unconditionally regardless of whether the
  slot was just nulled. `CheckControllerType` itself (0x7aea4c) has no null-check on
  `m_paJoystickDevices[deviceIndex]` — it dereferences the pointer directly to call
  `GetDeviceInfo` (vtable slot 0x78). Confirmed by reading the raw ctor disasm: both the
  release-and-null path (`0x7ae81c`-`0x7ae834`) and the immediately-following unconditional
  `bl CheckControllerType` (`0x7ae838`-`0x7ae840`) are reached by the SAME fallthrough, with no
  intervening branch on the now-nulled slot.
- **Impact:** a real latent null-pointer dereference during controller initialization, but only
  reachable when a joystick device successfully negotiates `SetDataFormat` yet then fails
  `SetCooperativeLevel` — an unusual, likely rare failure combination in practice (both calls
  target the same freshly-created device with no intervening state change), which is presumably
  why this was never caught.
- **Reproduction:** see the `// sic:` comment above the `CheckControllerType(i)` call in the ctor,
  `src/CExoRawInputInternal/CExoRawInputInternal.cpp`.

## `CSWSStore` — area-removal is a dead no-op, unlike every other game-object class

- **Where:** every other transcribed `CSWSObject`-derived class (`CSWSSoundObject`,
  `CSWSPlaceable`, `CSWSWaypoint`) implements `RemoveFromArea()` as `if (GetArea()) GetArea()->
  RemoveObjectFromArea(id); SetArea(0);`. `CSWSStore::RemoveFromArea()` (0x6b6c3c) is instead a
  bare `ret` — 4 bytes, no body at all. Its destructor (`~CSWSStore`, 0x6b6b74) does call
  `CSWSObject::GetArea()` at the expected point in the teardown sequence, but the RETURN VALUE
  is immediately discarded (the very next instruction loads an unrelated global) — no
  `RemoveObjectFromArea`/`SetArea(0)` call follows it anywhere in the function. Confirmed via
  both the raw disasm and Ghidra's own decompile (`CSWSObject::GetArea((CSWSObject *)this);` as a
  bare statement, result unused).
- **Impact:** a `CSWSStore` object removed from the game (destructed, or explicitly via
  `RemoveFromArea()`) never actually unregisters itself from its `CSWSArea`'s object list — the
  area keeps a dangling reference to it. Whether this is masked elsewhere (e.g. the area's own
  list being rebuilt/invalidated wholesale on module unload) is unconfirmed; reproduced
  faithfully either way since it's a real, verified shipped-binary behavior difference from every
  sibling class.
- **Reproduction:** `CSWSStore::RemoveFromArea()` is an empty function body; `~CSWSStore()` calls
  `((CSWSObject*)this)->GetArea();` and discards the result — see the `// sic:` comments in
  `src/CSWSStore/CSWSStore.cpp`.

## `CSWSDoor::SaveDoor` — redundant discarded `GetPortraitId()`, and a lost portrait-fallback branch

- **Where:** `CSWSDoor::SaveDoor` (0x539e10) calls the `GetPortraitId()` virtual (slot 53) TWICE in
  immediate succession — `blr` at 0x53a0e8 and again at 0x53a0f8 — discarding the first result and
  writing only the second. There is no branch of any kind between them.
- **Diagnosis:** the sibling `CSWSTrigger::SaveTrigger` (0x5ab86c) has the same two calls, but there
  the first feeds a real guard: `unsigned short portraitId = GetPortraitId(); if (portraitId !=
  0xffff) WriteFieldWORD(..., GetPortraitId(), "PortraitId"); else WriteFieldCResRef(...,
  GetPortrait(), "Portrait");`. The door version kept the local but lost the `if`, so the local is
  dead — clang drops it, but must still emit its call (an opaque virtual can have side effects).
- **Impact:** one wasted virtual call per door saved (harmless), but the real defect is the missing
  fallback: a door whose portrait id is 0xffff (== "none") saves that sentinel into the GFF
  "PortraitId" WORD instead of writing the "Portrait" resref field the trigger path would.
- **Reproduction:** a bare `GetPortraitId();` statement before the write — see the `// sic:` comment
  in `src/CSWSDoor/CSWSDoor.cpp`. (Byte-exact: the discarded call is what makes it match.)

## `CSWSDoor::SaveDoor` — the two trap-DC clamps are written inconsistently

- **Where:** `SaveDoor` saves both trap DCs difficulty-adjusted (Easy −5 / Difficult +5), each
  meant to be clamped to a minimum of 1. The two clamps are not the same test:
  - `DisarmDC` (0x53a3d0): `tst w8,#0xff; csinc ne` — an **`unsigned char` `== 0`** test.
  - `TrapDetectDC` (0x53a41c): `cmp w8,#1; csinc gt` — a **signed `> 1`** test on the full int.
- **Impact:** they disagree at the edges. For a base DisarmDC of 0..4 on Easy, the adjusted value is
  negative; the detect path clamps it to 1, but the disarm path's byte test sees a nonzero low byte
  (e.g. −5 → 0xfb) and passes it straight through, so the saved DisarmDC becomes 251. Only an exact
  wrap to 0 (base 5 on Easy, or base 251 on Difficult) trips the disarm clamp. The `CSWSTrigger`
  accessors `GetDisarmDC`/`GetDetectDC` (0x5aa53c/0x5aa4e4) both use the signed `> 1` form, and
  `CSWSPlaceable::SavePlaceable` (0x6a4db8) uses the unsigned-byte form for BOTH of its DCs
  (0x6a4f70 and 0x6a4fbc) -- i.e. every other site is self-consistent, and only SaveDoor mixes the
  two forms within one function.
- **Reproduction:** an `unsigned char disarmDC` + `if (disarmDC == 0) disarmDC = 1;` versus an
  `int detectDC` + `detectDC > 1 ? detectDC : 1` — see the `// sic:` comment in
  `src/CSWSDoor/CSWSDoor.cpp`.
- **Addendum (v459) — the same split runs through the READ accessors, and it is a RETURN TYPE.**
  `CSWSDoor::GetDisarmDC` @0x539b70 and `CSWSPlaceable::GetDisarmDC` @0x6a5790 return
  **`unsigned char`** (each ends `tst w8,#0xff; csinc w0,w8,wzr,ne`), while both classes'
  `GetDetectDC` returns **`int`** (`cmp w8,#1; csinc …,gt`). Confirmed from the caller side by
  `CSWSMessage::SendServerToPlayerExamineGui_MineData` @0x5f096c, which masks the door/placeable
  results (`and w8,#0xff`) and compares them UNSIGNED (`b.hi`) while comparing
  `CSWSTrigger::GetDisarmDC`'s genuine `int` SIGNED (`b.gt`).
  ⚠ **So the inconsistency is not confined to SaveDoor** — the byte-vs-int split is baked into the
  disarm/detect accessor pair itself on both classes. **Do not "unify" the four return types**; the
  asymmetry is what makes them match.

## `CSWCollisionMesh::RenderMesh` — the debug-draw stub tail-calls an unrelated getter

- **Where:** `_ZN16CSWCollisionMesh10RenderMeshEijj` (0x6b42f4) is 12 bytes in full:
  `ldr x8,[x0]; ldr x1,[x8,#0x38]; br x1`. It reads its own vptr and tail-dispatches vtable byte
  offset 0x38 — slot 7 — then returns whatever that returns.
- **Diagnosis:** `_ZTV16CSWCollisionMesh` (0xc24490) is 104B = offset-to-top + typeinfo + exactly
  11 slots and has no dtor pair, so slot 7 is `GetWalkableMaterialMask()` — a no-argument getter
  whose base implementation (0x35f538) is `mov w0,#-1; ret`. So `RenderMesh(int, unsigned int,
  unsigned int)` silently discards all three of its arguments (the flag and the two RGBA debug
  colours), draws nothing, and returns a material mask. Not identical-code folding: 0x6b42f4 has
  exactly one symbol. The real body was evidently `#ifdef`-ed out of the shipped NDEBUG build,
  and whatever remained degenerated into a tail call to a neighbouring virtual slot.
- **Impact:** every debug-draw call site is a no-op. `CSWSDoor::GetMeshBoundingBox` (0x5367e4)
  calls `RenderMesh(0, 0xffff0000, 0xff00ff00)` under the `nRenderDoorBBox` toggle (0xc8cce0), so
  enabling that toggle in the shipped build draws the bounding corners and wireframe but never the
  mesh itself.
- **Reproduction:** not yet transcribed — the caller's `bl` is masked, so `CSWSDoor::
  GetMeshBoundingBox` matches against the plain `void RenderMesh(int, unsigned int, unsigned int)`
  declaration in `src/CSWCollisionMesh/CSWCollisionMesh.h` regardless. Reproducing the stub's own
  12 bytes needs `return GetWalkableMaterialMask();` as the body.

## `CSWSJournal::SetState` — a new pending-update record ORs into uninitialized stack

- **Where:** `_ZN11CSWSJournal8SetStateERK10CExoStringji` (0x67a4f0), at 0x67ad9c:
  `ldrh w8,[sp,#0x118]; ldr w9,[sp,#0x40]; orr w8,w8,#0x8000; cmp w9,#0; csel w8,w8,w19,eq;
  strh w8,[sp,#0x118]`.
- **Diagnosis:** `sp+0x118` is the function's stack-local `CSWSPlayerJournalQuestUpdates update`,
  the record appended when the player has no pending delta for this quest yet. That class's default
  constructor is IMPLICIT and its `m_updateFlags` is a plain scalar, so `new`/stack construction
  leaves it uninitialized — the only ctor call the disasm makes on the object is
  `CExoString::CExoString()` for the tag member at +0x08 (0x67a568). Nothing writes `sp+0x118`
  between that ctor and this `ldrh`. So on the `bAlreadyPresent == 0` path the engine reads
  uninitialized stack, ORs `0x8000` into it, and stores the result — which is then copied into the
  live list by `Add`. The sibling path is clean (`update.m_updateFlags = nFlags`, a plain
  assignment via the `csel`'s other arm), which is what makes this look like a `|=`-for-`=` typo
  rather than a deliberate accumulate.
- **Impact:** the appended record's flag word carries whatever garbage that stack slot last held,
  above and beyond the intended `0x8000` ("quest is new"). The slot is heavily reused within
  SetState itself, so the value is not arbitrary in practice — but any of the other delta bits
  (0x0004 date / 0x0008 time / 0x0100 / 0x4000 destroyed) can end up spuriously set, telling the
  client to re-render fields that did not change. It is masked in the common case because 0x8000
  supersedes the per-field bits anyway: the client needs the whole entry when the quest is new.
  Note `nFlags` already contains `0x8000` on this exact path (0x67a670), so the intended `= nFlags`
  would have been equivalent-and-correct.
- **Reproduction:** the `// sic:` comment in `src/CSWSJournal/CSWSJournal.cpp` — `update
  .m_updateFlags |= 0x8000;` on the `!bAlreadyPresent` branch. Writing the honest `= 0x8000` emits
  a plain `mov w8,#0x8000` and loses the `ldrh`+`orr`+`csel`.

## `CSWRules::LoadDifficultyInfo` — a 4-byte 2DA out-param written into a 1-byte local

- **Fixed under `K2_FIX_ENGINE_BUGS`** (a 4-byte slot). This bug is real on the host: at `-O0` the
  stray bytes overwrite the loop counters and the loop never terminates.

- **Where:** `_ZN8CSWRules18LoadDifficultyInfoEv` (0x53435c). The out-param is passed as
  `add x3, sp, #4` to `C2DA::GetINTEntry(int, int, int*)` (30 calls, unrolled 6-wide over 5
  iterations), and the result is read straight back as a BYTE: `ldrb w8, [sp, #4]`.
- **Diagnosis:** the `ldrb` is the whole finding, and it is not a codegen artifact — LLVM does not
  narrow an `i32` load to `i8` just because only the low byte is used (it widens, if anything), so
  the load was `i8` in the original's IR, which means the *source* read a one-byte lvalue at that
  address. Since every `GetINTEntry` overload in the dynsym takes `int*` (there is no narrower
  one), the only shape that produces it is a one-byte local whose address is cast:
  `unsigned char nValue; ... GetINTEntry(nOption, nDifficulty + 1, (int*)&nValue)`. `GetINTEntry`
  then writes four bytes into a one-byte object — three bytes past its end.
- **Impact:** benign in this build by luck of layout, not by design. `sp+4` sits in a 4-byte hole
  before the `CResRef` temporary at `sp+8` (the stack canary is at `sp+0x18`), so the three
  overflow bytes land on padding. Any layout change — a different local, a different compiler, a
  different optimization level — puts a live object at `sp+5..7` and this silently corrupts it.
  Note the write happens on the failure path too: `GetINTEntry` writes its out-param before
  returning 0, and the `csel` discards the value afterwards rather than the store.
- **Reproduction:** the `// sic:` comment in `src/CSWRules/CSWRules.cpp`. Writing the honest
  `int nValue;` + `(unsigned char)nValue` is length-exact but emits `ldr w8` for all six loads —
  DIFF(12), i.e. this bug is worth exactly 2 bytes per call site and nothing else.

## `CSWRules::IsFeatUseable` — dereferences a pointer it just conditionally nulled

- **Where:** `_ZN8CSWRules13IsFeatUseableEht` (0x5346e0):
  `ldrb w9,[x8,#0x28]; tst w9,#0x10; csel x8,xzr,x8,eq; ldrb w8,[x8,#0x2e]`.
- **Diagnosis:** `+0x28` bit 4 is `CSWFeat`'s "row is present in feat.2da" flag —
  `CSWFeat::CSWFeat` (0x35eb58) defaults it SET (`and w8,w8,#0xe0; orr w8,w8,#0x10`) and
  `CSWRules::LoadFeatInfo` CLEARS it (`and w9,w9,#0xffffffef`, 0x531ef0) for every row whose
  "Feat" column lookup fails. The `csel` nulls `pFeat` when that bit is clear and the very next
  instruction reads `pFeat->m_nMinWIS` unconditionally, i.e. a load from address `0x2e`. The
  sibling `GetFeat` (0x53470c) performs the identical present-bit test and correctly RETURNS the
  null instead of dereferencing it, which is what shows this to be an omitted null check rather
  than a deliberate idiom.
- **Impact:** a null-pointer read — a crash, not silent corruption — for any feat whose row is
  absent from feat.2da. Unreachable as long as every feat id a caller passes is a populated row,
  which is why it shipped. Note `IsFeatUseable` also skips `GetFeat`'s *bounds* check
  (`m_nFeats <= nFeat`) entirely, so an out-of-range id indexes past `m_paFeats` before even
  reaching this point.
- **Reproduction:** the `// sic:` comment in `src/CSWRules/CSWRules.cpp` — the conditional null
  followed by an unconditional dereference. This is the shape clang compiles to the `csel`; an
  honest early `return 0;` would emit a branch instead.
- **Fixed under `K2_FIX_ENGINE_BUGS`** (warnings ledger, 2026-10-22): GetFeat's `m_nFeats <= nFeat`
  bounds check is added up front and an absent row returns 0 instead of reading through the null.

## `CSWCCreatureStats::GetMinSpellRange` — returns an uninitialized float on an unknown range code

- **Where:** `_ZN17CSWCCreatureStats16GetMinSpellRangeERK10CExoString` (0x372ebc). The body maps a
  spells.2da "Range" code (single-letter `T`/`P`/`S`/`M`/`L`/`W`) to a preloaded float on the rules
  object (`g_pRules` +0x24/+0x28/+0x2c/+0x30/+0x34). Each hit computes a pointer and the five
  converge on one shared `ldr s0,[x8]`; the no-match path (`b 0x372fb4 -> b 0x372f08`) branches to
  the epilogue **past** that shared load, so `s0` is whatever the return register happened to hold.
- **Diagnosis:** the function has no default value — it relies on every caller passing a code that
  is one of the six. Any other string returns garbage.
- **Impact:** an uninitialized-read return; benign only because the callers (spell UI/targeting)
  always source the code from a populated spells.2da "Range" cell.
- **Reproduction:** the `// sic:` in `src/CSWCCreatureStats/CSWCCreatureStats.cpp` — a `float*
  pRange;` selection with a separate `else return fUnknown;` of an uninitialized local. The explicit
  return is load-bearing: if the function simply runs off the end, clang treats the fall-through as
  UB and **elides the final `"W"` comparison** (assumes it always matches), which is −8B. The
  uninitialized-return keeps the branch; the pointer-selection (vs `return field;` per branch) is
  what produces the single shared load instead of five inline ones.

## `CSWCCreature::SetPrecalculatedPosition` — the 0.03f wall-slide arm's cross-product middle term

- **Where:** `_ZN12CSWCCreature24SetPrecalculatedPositionERK6Vector` (0x49ac20), the sign-flip
  (subtype-2) wall-slide arm. The slide offset is a componentwise cross of the global `zaxis`
  against the normalized to-target direction: `cx = zaxis.y*dir.z - zaxis.z*dir.y`, `cz =
  zaxis.x*dir.y - zaxis.y*dir.x` — both textbook `cross(zaxis, dir)` terms — but the middle
  component computes `cy = zaxis.z*dir.x - dir.z*dir.y` (disasm: `fmul v1.2s, v5.2s, v1.2s` with
  lanes (dir.y*zz, dir.z*dir.y), then `fsub v1.2s, v4.2s, v1.2s` at 0x49b51c-0x49b520). The
  second product should be `zaxis.x*dir.z` for a real cross product; the shipped code multiplies
  the direction's own z and y. The parallel 0.06f arm (subtype-2, no sign-flip) computes the
  correct `zaxis.z*dir.x - zaxis.x*dir.z` (0x49b5cc-0x49b5d4).
- **Impact:** the wall-slide nudge applied on a sign-flip iteration points slightly off the true
  tangent plane whenever the approach direction has both y and z components; cosmetic at worst
  (it's a 0.03-scale debug-era slide nudge retried up to 6 times, and the loop re-tests with
  TestDirectLine every iteration).
- **Reproduction:** the `// sic:` on `cy` in the bFlip arm of
  `src/game/clientcore/CSWCCreature/CSWCCreature.cpp`.

## `CSWCCreature::EvaluateNewWayPoints` — the truncate arm copies one element past the source array

- **Where:** `_ZN12CSWCCreature20EvaluateNewWayPointsEiP6VectoriRKS0_` (0x49986c), the arm that
  scans `pPoints[1..nCount-1]` for the first way-point with a clear TestDirectLine from the
  creature's position and drops everything before it. On a hit at scan index `i` (0-based over
  `pPoints+1`), it allocates `nCount - i` Vectors and copies `nCount - i` elements starting AT
  the clear point `pPoints[i+1]` (disasm: `subs w8, w20, w24` for the count, the running source
  pointer `x23` for the start, `memcpy(pNew, x23, (nCount-i)*12)` at 0x499b3c-0x499b48). The
  source array's last valid element is `pPoints[nCount-1]`, but the copy reads through index
  `i+1 + (nCount-i) - 1 = nCount` — 12 bytes past the end of the caller's array. The intended
  "keep from the clear point onward" count is `nCount - i - 1`.
- **Impact:** a genuine 12-byte heap over-read in the shipped build; the over-read element
  becomes the new list's last way-point, so the creature may walk to garbage coordinates
  transiently (until the next evaluation rebuilds the list).
- **Reproduction:** the `// sic:` on the truncate arm's `memcpy` in
  `src/game/clientcore/CSWCCreature/CSWCCreature.cpp`.

## `CSWCArea::RestartMiniGame` — the new minigame object leaks when `Load()` fails

- **Where:** `CSWCArea::RestartMiniGame` @0x4aa210.
- **What:** the function `new`s a replacement `CSWMiniGame`, re-AddTracks/AddObstacles the old
  one's contents, and on `pNew->Load() == 0` returns 0 WITHOUT deleting `pNew` (the failure path
  branches straight to the return — no dtor, no `operator delete`). The success path deletes the
  OLD game and installs the new one, so only the failure arm leaks.
- **Impact:** a 0x158-byte heap leak per failed minigame restart (plus whatever Load() partially
  allocated before failing).
- **Reproduction:** the `⚠ the orig LEAKS pNew` comment on the transcription in
  `src/game/clientcore/CSWCArea/CSWCArea.cpp` (v363).

## `CSWCArea::AddPlaceableObjectLight` — the fetched light color is never applied

- **Where:** `CSWCArea::AddPlaceableObjectLight` @0x4a70e4.
- **What:** the function looks up the placeable's `LightColor` row in LightColor.2da and reads
  its `RED`/`GREEN`/`BLUE` columns into a local `vColor` — which is then NEVER read. No
  SetColor-style call exists anywhere in the body (the only color-relevant vtable call,
  `SetPartLocalPosition`, gets position + quaternion). The light always keeps its model default
  color.
- **Impact:** the per-placeable light-color table is dead data; every placeable light renders in
  the default color regardless of the 2DA.
- **Reproduction:** the `vColor is NEVER consumed` comment on the transcription in
  `src/game/clientcore/CSWCArea/CSWCArea.cpp` (v364).

## `CSWCArea::AddPlaceableObjectLight` — found-path light-creation failure deletes the still-linked array entry

- **Where:** `CSWCArea::AddPlaceableObjectLight` @0x4a70e4.
- **What:** when an entry for the object id already exists in `m_placeableObjectLights` and the
  replacement `NewCAurObject` call fails, the failure arm does `delete pEntry; return 0;` — but
  `pEntry` is still linked in `m_placeableObjectLights` (it is only unlinked by
  `RemovePlaceableObjectLight`). The array is left holding a dangling pointer with the count
  unchanged.
- **Impact:** latent use-after-free: any later `GetPlaceableObjectLight`/
  `RemovePlaceableObjectLight` scan (or the area dtor) dereferences the freed entry. Only
  reachable when a light is re-added for an object that already has one AND model loading fails.
- **Reproduction:** the `⚠ Second engine bug` comment on the transcription in
  `src/game/clientcore/CSWCArea/CSWCArea.cpp` (v364).

## `CSWCArea::AddPlaceableObjectStatic` — the built facing quaternion is never used

- **Where:** `CSWCArea::AddPlaceableObjectStatic` @0x4a757c.
- **What:** after `SetPosition(vPos)` and `SetOrientation(vFacing)` (the raw by-value Vector),
  the function computes `YawPitchRoll(Yaw(vFacing), 0.0f, 0.0f)` — a full
  euler-extract-then-rebuild round trip — and DROPS the result: the very next instruction
  reloads `d0` from the constant pool for the +0x16c store, clobbering the returned quaternion
  in s0..s3 with no consumer in between (the macOS oracle build shows the same dead call).
  YawPitchRoll is not a pure function from the compiler's view, so the call survives in the
  shipped binary as a genuine dead call.
- **Impact:** wasted cycles only; the placeable's facing comes from the raw Vector via
  SetOrientation and the quaternion goes nowhere.
- **Reproduction:** the `// sic:` comment on the transcription in
  `src/game/clientcore/CSWCArea/CSWCArea.cpp` (v365).

## `CSWCCreature::DoFootstepVisual` — invalid surface type leaks the visual effect

- **Where:** `CSWCCreature::DoFootstepVisual(unsigned char nType, int)` (0x48df58). The
  `CSWCVisualEffect` is heap-allocated (`new CSWCVisualEffect(1)`) BEFORE the `switch (nType)`
  that picks the effect resref; the `default` arm (any nType other than 0/1/2) returns
  immediately with no `delete` — the only operator-delete in the function is the EH cleanup
  pad for a ctor throw (bltally: `_ZdlPv` ×1, in the pad, not on the normal path).
- **Impact:** a 0x180-byte leak per call with an out-of-range nType; unreachable from
  `PlayFootstep` (it only ever passes 0/1), so dead in practice.
- **Reproduction:** the `default: return;` switch arm in the transcription — see the NOTE in
  `src/game/clientcore/CSWCCreature/CSWCCreature.cpp` (v369).

## `CSWSMessage::TestPartyObjectUpdateDifferences` — the party-snapshot search advances the WRONG list

- **Where:** `CSWSMessage::TestPartyObjectUpdateDifferences(CSWSPlayer*, CSWSCreature*,
  CLastUpdatePartyObject**, unsigned int*)` @0x5ef9f8.
- **What:** the search for an existing party-bar snapshot record reads each element with
  `GetAtPos` on the PARTY list at `CSWSPlayer+0x18`
  (`m_pLastUpdatePartyObjectList`) but advances the cursor with `GetNext` on the OBJECT list at
  `+0x10` (`m_pLastUpdateObjectList`). Both builds do it (arm64 `ldr x8,[x23,#0x18]` for the
  GetAtPos receiver vs `ldr x8,[x23,#0x10]` for the GetNext receiver; macOS `0x18(%rbx)` vs
  `0x10(%rbx)`), so it is source, not a decoding error.
- **Impact:** the walk steps through the object list's node chain while dereferencing nodes as if
  they came from the party list. In practice the two lists are usually short and the loop simply
  terminates early or mis-matches, so a party member's record is re-allocated more often than it
  should be (each miss costs a `new`(0x78) plus a full seed-mask resend). It does not corrupt
  memory, because `GetAtPos` is applied to whatever node the object list yields and both lists
  hold heap records of their own.
- **Reproduction:** the `⚠ THE BUG` comment on the transcription in
  `src/game/servercore/CSWSMessage/CSWSMessage.cpp` (v463).

## `CLastUpdatePartyObject` — the party snapshot's Z position is never refreshed

- **Where:** the pair `CSWSMessage::TestPartyObjectUpdateDifferences` @0x5ef9f8 (reader) and
  `CSWSMessage::StoreValuesInLastUpdatePartyObject` @0x5efe48 (writer).
- **What:** the snapshot record holds three position floats at +0x48/+0x4c/+0x50, and Test compares
  all three against the live `CSWSCreature::m_position` to decide whether the party bar needs a
  position update (squared distance vs `4.0f`). The writer refreshes only `.x` (+0x48) and `.y`
  (+0x4c) — nothing in either build ever stores to +0x50 after
  `TestPartyObjectUpdateDifferences` zero-initializes it on first sight.
- **Impact:** the Z term of that distance is permanently `0 - m_position.z`, i.e. measured against
  the world origin rather than against the last replicated Z. Any party member standing at
  |z| >= 2.0 has the position bit raised on EVERY frame, so mask bit 9 is effectively always set
  for them and the position is re-sent continuously. Wasted bandwidth, not a visible fault.
- **Reproduction:** the `⚠⚠ ENGINE BUG` comment on `m_fUnk0x50` in
  `src/game/servercore/CLastUpdatePartyObject/CLastUpdatePartyObject.h` (v463).

## `WriteGameObjUpdate_MajorGUIPanels` — the open container's repository can be an uninitialized pointer

- **Where:** `CSWSMessage::WriteGameObjUpdate_MajorGUIPanels` @0x5e9380, the branch taken when the
  player has a container open (`*CSWSPlayer::m_pUnk88 != CGameObjectArray::INVALIDOBJID`).
- **What:** the container object is resolved out of the game-object array, then its repository is
  selected by TWO SEQUENTIAL tests on `m_objectType` — one for `OBJECT_TYPE_ITEM` (reading +0x388)
  and one for `OBJECT_TYPE_PLACEABLE` (reading +0x558). Neither arm has an `else`, and the local
  is never initialized. When the resolved object is NEITHER an item nor a placeable, whatever was
  in the register is handed to `CSWSMessage::WriteRepositoryUpdate` as its `CItemRepository*`.
  Both builds do it (arm64 leaves x3 undefined on that path; the macOS oracle leaves %rcx
  undefined the same way), so it is source, not a decoding error.
- **Impact:** `WriteRepositoryUpdate` immediately dereferences the pointer (`ldr w8,[x3,#0x18]`),
  so the path is a wild read. It is unreachable in practice only because the open-container id is
  set exclusively by the two GUI handlers that open an item or a placeable — the invariant is real
  but it is maintained entirely outside this function and nothing here checks it.
- **Reproduction:** the `⚠ pRepository is deliberately left UNINITIALIZED` comment on the
  transcription in `src/game/servercore/CSWSMessage/CSWSMessage.cpp` (v464).

## `WriteGameObjUpdate_PlayerUpdate` — two player-panel groups ship nothing but constants

- **Where:** `CSWSMessage::WriteGameObjUpdate_PlayerUpdate` @0x5eb6d4, mask groups `0x80` and
  `0x20`. Both builds emit them identically, so this is the shipped source and not a decoding gap.
- **What:** group `0x80` declares a two-entry value array and a two-entry flag array, zero-inits
  both, and then scans the value array for non-zero entries to build a count it ships with
  `WriteBYTE`. **Nothing ever writes the value array between the zero-init and the scan**, so the
  count is always 0 and the group's second pass — the one that would actually ship per-class
  entries — is unreachable. Group `0x20` is simpler still: a two-trip loop writing four literal
  zero WORDs, reading no state at all.
- **Impact:** benign on the wire (the client reads a well-formed zero-length group in both cases),
  but the `0x80` group is dead weight — the sibling `0x40` group directly below it is the same
  shape *with* its value array populated from `GetSpellsPerDayLeft`, which is what `0x80` looks
  like it was meant to be. Neither bit is ever raised by
  `TestPlayerUpdateDifferences` @0x5ead6c, so in practice neither group runs at all; they are
  reachable only if some other producer sets those bits.
- **Reproduction:** the `⚠⚠ WRITE-ONLY AND VESTIGIAL` and `⚠⚠ WRITE-ONLY AND CONSTANT` comments on
  the transcription in `src/game/servercore/CSWSMessage/CSWSMessage.cpp` (v475).

## `CSWSCreature::CanEquipMiscellaneous` — passes the raw `pnSlot` POINTER as `CheckProficiencies`' slot arg

- **Where:** `CSWSCreature::CanEquipMiscellaneous(CSWSItem*, unsigned int* pnSlot, int, int)`
  (0x5425a8). Its first call is `CheckProficiencies(pItem, *pnSlot)` per the mangling
  (`...EP8CSWSItemj`), but neither build dereferences the pointer: arm64 passes `w2` through
  unchanged (no `ldr w2,[x22]` anywhere before the call), and the x86 oracle passes `%rdx`
  through unchanged as well. `CheckProficiencies` therefore receives the heap pointer's low 32
  bits as its "slot" argument. (The later `GetItemInSlot(*pnSlot)` DOES dereference — so the
  omission is specific to the CheckProficiencies call.)
- **Impact:** depends on what CheckProficiencies (0x542648, still untranscribed) does with the
  argument; if it treats it as an inventory slot the proficiency check is effectively garbage
  (likely benign in practice if the value indexes a masked table or is unused).
- **Reproduction:** the call spells the truncation explicitly —
  `CheckProficiencies(pItem, (unsigned int)(unsigned long long)pnSlot)` — see the `// sic:`
  comment in `src/game/servercore/CSWSCreature/CSWSCreature.cpp`. Found v537; the byte-match
  fails by one instruction with the "correct" `*pnSlot` spelling.

## `CSWSCreature::SendFeedbackMessage` — deletes caller-owned `CSWCCMessageData` on every path

- **Where:** `CSWSCreature::SendFeedbackMessage(unsigned short strref, CSWCCMessageData* pData)`
  (0x621a10). When the player resolves, a null pData is heap-allocated, filled and sent — but the
  trailing `if (pData != 0) delete pData;` runs regardless of who allocated it (arm64 0x621a98
  falls into dtor + `operator delete` on the caller-supplied pointer too).
- **Impact:** any caller passing its own bag loses ownership (use-after-free if it touches the
  bag afterwards). The one known caller pattern (`RequestBuy` etc.) passes 0, which is safe.
- **Reproduction:** transcribed as-is in `src/game/servercore/CSWSCreature/CSWSCreature.cpp`
  (v539), byte-level `bl` sequence pins the unconditional delete.

## `CSWSCreature::CanUnEquipWeapon2` — LUO snapshot clears the wrong slot indices

- **Where:** `CSWSCreature::CanUnEquipWeapon2(CSWSItem*)` (0x54efa4). The function tests the
  creature-weapon slots 0x40000/0x80000 (slot bits 18/19) but the LUO-inventory snapshot clear
  stamps `m_slotItemId[15]` and `m_slotItemId[16]` (+0x78/+0x80 in BOTH the arm64 build and the
  x86 oracle) — the indices consistent with the tested slots would be [18]/[19].
- **Impact:** the client's inventory GUI keeps stale object ids for the two creature-weapon
  slots after an unequip; cosmetic/desync-class. The sibling `CanUnEquipWeapon` (0x54eee0) clears
  [4]/[5] == its tested bits 0x10/0x20, i.e. correct.
- **Reproduction:** transcribed as-is in `src/game/servercore/CSWSCreature/CSWSCreature.cpp`
  (v539); the matched twin shows the intended pattern.

## `CSWSCreature::SaveAttackerList` — writes fields into list elements never added to the list

- **Where:** `CSWSCreature::SaveAttackerList(CResGFF*, CResStruct*)` (0x63f5f8). The loop over
  m_pAttackerList calls `WriteFieldDWORD(&elem, id, "ObjectId")` on a scratch CResStruct WITHOUT
  ever calling `AddListElement(&elem, &list, i)` first — no such call exists in the binary
  (verified with raw objdump, not just the decompiler). It also writes only the LOW 32 bits of
  the attacker's 64-bit object id (`ldr w2,[x0,#8]`).
- **Impact:** the saved "AssociateList" is corrupt/empty in the original — recent-attacker state
  does not survive a save/load round-trip.
- **Reproduction:** transcribed as-is in `src/game/servercore/CSWSCreature/CSWSCreature.cpp`
  (v539).

## `CSWSCreature::CleanInvitationLists` — feedback message sent through a possibly-null pointer

- **Where:** `CSWSCreature::CleanInvitationLists()` (0x552b4c), the m_pInvitationsOffered reaping
  loop. `GetCreatureByGameObjectID` may return null; the code null-tests it only to guard
  `SetInvitedToParty(0)`, then falls through and calls
  `pCreature->SendFeedbackMessage(0x2b, pData)` on the SAME pointer with no re-test.
- **Impact:** null dereference when a stale party invitation names a creature that has since been
  destroyed (area unload / death). Crash-class, not cosmetic.
- **Confirmed in both builds:** arm64 keeps the possibly-null pointer live in x20 across the
  inlined DelIndex block; the x86 oracle spills it to -0x48(%rbp) and reloads it. Neither re-tests.
- **Reproduction:** transcribed as-is (byte-matched) in
  `src/game/servercore/CSWSCreature/CSWSCreature.cpp` (v550).
- **Fixed under `K2_FIX_ENGINE_BUGS`** (v2251 triage): the feedback is sent only when the creature
  still exists (K1 is the same, @0x43ac4c).

## `CSWSCreature::AIActionCheckMoveToPoint` — passes an int's VALUE where every sibling passes its address

- **Where:** `AIActionCheckMoveToPoint(CSWSObjectActionNode*)` (0x541a50), the 6th
  `AddActionToFront` argument pair. The action node's +0x60 int is passed as
  `(void*)(long)nParam` — the sign-extended VALUE cast to a pointer (`ldrsw` on arm64, `movslq`
  on x86) — where every sibling AIActionCheck* passes `&node->field`. A missing `&` in the
  shipped source.
- **Impact:** the queued action receives a garbage pointer in that slot; whether it is ever
  dereferenced depends on the action handler, so severity is unclear but it is certainly not
  what was intended.
- **Reproduction:** transcribed as-is in `src/game/servercore/CSWSCreature/CSWSCreature.cpp`
  (v551). Spelled `(void*)(long)nParam` — measured codegen-identical to `(void*)nParam` (the
  `ldrsw` comes from the int->pointer sign-extension either way) and it silences
  `-Wint-to-void-pointer-cast`. Note the sibling `AIActionCheckMoveToPointRadius` @0x541cfc
  passes the ADDRESS of the same +0x60 slot, which is what makes this a slip rather than a
  deliberate tagged-pack convention.

## `CSWSCreature::CheckItemAttributeRestrictions` — the INT arm checks Constitution

- **Where:** `CheckItemAttributeRestrictions(CSWSItem*)` (0x5432fc), the ability-score switch on
  the type-0x45 property's `m_subType`. Cases 0/1/2/4/5 read `m_pStats->m_n{STR,DEX,CON,WIS,CHA}
  Base` at +0x139/+0x13b/+0x13d/+0x141/+0x143; **case 3 (INT) reads +0x13d — Constitution —
  instead of +0x13f.** A copy-paste slip in the shipped source.
- **Impact:** an item carrying an "Intelligence required" use limitation is gated on the wearer's
  CON score instead. Silently lets low-INT / high-CON creatures equip restricted items and blocks
  the reverse. Gameplay-visible, not a crash.
- **Confirmed in both builds:** the arm64 jump table at 0x9c9f3e gives cases 2 and 3 the SAME
  target (0x543450, `add x8, x8, #0x13d`); the x86 oracle's table likewise routes both to the
  `movzbl 0x13d(%rax)` arm. `m_nINTBase` @+0x13f is never read by this function.
- **Reproduction:** transcribed as-is (byte-matched) in
  `src/game/servercore/CSWSCreatureAI/CSWSCreatureAI.cpp` (v555), with the duplicated case arm
  written out explicitly and marked `// sic`.

## `CSWSPlaceable::EventHandler` — `pAttacker` is used uninitialized when the melee payload is NULL

- **Where:** `EventHandler(...)` (0x6a5ae0), the `EVENT_ON_MELEE_ATTACKED` arm. `pWeapon` is
  initialized to 0; the attacker pointer is **not**. Everything that assigns it lives inside
  `if (pAttack != 0) { … }`, but the `if (pAttacker != 0) { BroadcastCombatStateToParty(…);
  RemoveCombatInvisibilityEffects(); … }` block below runs unconditionally.
- **Impact:** a melee-attacked event delivered with a NULL `pData` reaches that block with x22
  still holding `nEventType` (15), so the null test passes and the engine calls two creature
  methods through a pointer of 0x0000000f. Crash.
- **Confirmed in both builds:** arm64 emits no `mov x22, xzr` on the `cbz x19` path into the shared
  continuation at 0x6a5ddc; the macOS oracle likewise never zeroes its register on that edge. The
  sibling `CSWSDoor::EventHandler` @0x536cfc *does* initialize it (`mov x22, xzr` is present), so
  this is a slip in one of the two copies of an otherwise copy-pasted arm.
- **Reproduction:** transcribed as-is in
  `src/game/servercore/CSWSPlaceable/CSWSPlaceable.cpp` (v601) — the declaration carries no
  initializer and a `⚠` note. That single missing instruction is what pins the function's length.

## `CSWSPlaceable::EventHandler` — the SPELL_IMPACT payload is freed without its destructor

- **Where:** the `EVENT_SPELL_IMPACT` arm frees its `CSWSSpellScriptData*` with a bare
  `operator delete`, with no destructor call. The payload holds a `CExoString m_sScript` at +0x30
  (pinned by `CSWSSpellScriptData::SaveData`'s `WriteFieldCExoString` on that offset).
- **Impact:** the script-name string's heap buffer leaks on every resolved spell/force-power
  impact against a placeable.
- **Confirmed in both builds:** arm64 branches straight to the shared `bl _ZdlPv` tail at 0x6a634c,
  skipping the `~CScriptEvent`/`~CSWCCMessageData` predecessors the other arms go through; the
  macOS oracle jumps to the same delete-only tail at 0x10039cf7e. The `EVENT_OPEN_OBJECT` arm
  shares that tail legitimately (its payload is a bare `unsigned long long`).
- **Reproduction:** transcribed as-is (`operator delete(pImpact);` with a `// sic`) in the same TU.

## `CSWCModule::AdjustCameraClipRange` — the near-plane argument is never used

- **Where:** `_ZN10CSWCModule21AdjustCameraClipRangeEff` @0x3d8518. The function takes two floats
  and applies only the SECOND one, to both the camera's far clip distance and the scene's far fog
  distance: `SetClipDist(fNear, fFar + fFarAdjust)` / `SetFogRange(fNear, fFar + fFarAdjust)`.
- **Impact:** callers that pass a near-plane adjustment get it silently dropped. The first
  parameter is dead on arrival — `mov v8.16b, v1.16b` saves only `s1`, and `s0` is clobbered by
  the first `ldp` before any use.
- **Confirmed in both builds:** the macOS slice has no `AdjustCameraClipRange` symbol, so this rests
  on arm64 alone — but a *parameter register overwritten before any read* is unambiguous, not a
  scheduling question.
- **Reproduction:** transcribed as-is in `src/game/clientcore/CSWCModule/CSWCModule.cpp` (v609);
  the unused parameter is named and flagged with a `⚠` in both the header and the definition.

## `CSWCModule::UpdateCameraBumpAndShake` — the random shake sign is 0 or −1, never +1

- **Where:** `_ZN10CSWCModule24UpdateCameraBumpAndShakeEf` @0x3dc5d0. Both shake components are
  scaled by `(rand_wincompatible() + 1) % 2 - 1`.
- **Impact:** `rand()` is non-negative, so `(r+1) % 2` is 0 or 1 and the expression yields −1 or 0.
  The intended random SIGN (±1) never comes out positive, so the camera shake only ever pushes yaw
  and pitch in one direction, and half the frames it does not shake at all.
- **Confirmed in both builds:** arm64 lowers it as `add w8,w0,#1 / add w9,w0,#2 / cmp w8,#0 /
  csinc / mvn / orr #1 / add` (the signed `% 2` idiom on `r+1`) and then `scvtf`; the macOS oracle
  emits the identical `leal 0x1(%rax) / shrl $0x1f / andl $-2 / subl / decl` sequence. Both compute
  `(r+1) % 2 - 1`, so the slip is in the shared source, not one port.
- **Reproduction:** transcribed as-is in the same TU with a `⚠` note.

## `CSWSEncounter::BuildSpawnList` — the empty-spawn-list early return leaks the temporary array

- **Where:** `_ZN13CSWSEncounter14BuildSpawnListEv` @0x59c3fc. The function allocates a temporary
  `new CEncounterListEntry[m_nMaxCreatures]`, has `BuildInitialSpawnList` fill it, and frees it with
  `_ZdaPv` @0x59c5f4 on the way out. But the `if (nSpawnCount == 0)` guard immediately after the
  fill is compiled as `cbz w25, #0x59c5fc` — and 0x59c5fc is the **epilogue**, four instructions
  *past* the free.
- **Impact:** every activation of an encounter whose roster comes back empty leaks
  `m_nMaxCreatures * 0x28 + 8` bytes. An encounter with no SingleSpawn slots and a zero
  GuaranteedCount everywhere reaches this on every re-arm.
- **How it is distinguishable from a compiler transform:** the same value is *also* tested by the
  loop's own `cmp w25, #1 / b.lt` a few instructions later, at a *different* target. Two guards on
  one value with two destinations is only reachable from two source-level exits; and the `!= 0`
  spelling (rather than `<= 0`) is what leaves the second guard non-redundant, since a negative
  count is not excluded by it.
- **Reproduction:** transcribed as-is (early `return`, no `delete[]`) with a `⚠` note in
  `src/game/servercore/CSWSEncounter/CSWSEncounter.cpp` (v619). Restores the function to
  length-exact.

## `CSWSEncounter::BuildInitialSpawnList` — the "already spawned" latch is set on a local copy

- **Where:** `_ZN13CSWSEncounter21BuildInitialSpawnListEP19CEncounterListEntryRi` @0x59c644. Pass 1
  walks `m_paSpawnPoints`, copies each element into a local `CEncounterListEntry entry`, and for a
  SingleSpawn slot that has not been used yet (`entry.m_Unk0x18 == 0`) appends one creature and then
  arms the latch with `entry.m_Unk0x18 = 1` — on the **copy**, not on `m_paSpawnPoints[i]`.
- **Impact:** the store is dead. The next iteration overwrites the whole local from the next array
  element, so the latch is never observed by anything; the intended "don't re-spawn this slot"
  bookkeeping does nothing. (Pass 1 already visits each slot exactly once, so the visible behaviour
  is unchanged — but the field it was meant to persist is the one `CEncounterSpawnPoint::operator=`
  goes out of its way to reset on every reorder, so the omission was not intentional.)
- **Confirmed in both builds:** arm64 emits `mov w8, #1 / str w8, [sp, #0x28]` into the local's
  frame slot; the macOS oracle emits `movl $0x1, -0x40(%rbp)` into its own. Same store, same
  deadness, so the slip is in the shared source.
- **Reproduction:** transcribed as-is with a `⚠` note in the same TU (v619).

## `CSWSEncounter::CSWSEncounter` — the shared power-lookup table is refilled on every construction

- **Where:** `_ZN13CSWSEncounterC2Ey` @0x59a880. The tail of the constructor fills the FILE-STATIC
  `CSWSEncounter::m_sPowerLookup` (`_ZN13CSWSEncounter14m_sPowerLookupE`, a
  `CExoArrayList<float>`) with `for (i = 0; i < 0x33; i++) m_sPowerLookup.Add((float)pow(1.5, i));`
  — and there is **no guard**. The loop's preheader reads the list's CURRENT count
  (`ldr w28,[x27,#8]`) and appends 51 more entries to whatever is already there; the only compare
  in the loop is the induction variable against `#0x33`.
- **Impact:** unbounded growth of a process-lifetime static. Constructing N encounters leaves
  `51 * N` floats in the list, with a capacity-doubling realloc + copy every time it fills. A module
  with a few hundred encounters churns through several reallocations of a table that is only ever
  read at indices `[0..50]` (`CalculatePointsFromCR`'s integer-CR path), so the extra entries are
  pure waste rather than a wrong answer.
- **Confirmed in both builds:** the macOS oracle's constructor has the same unguarded loop
  (`xorl %ebx,%ebx / cvtsi2sd / callq _pow / callq __ZN13CExoArrayListIfE3AddEf / incl %ebx /
  cmpl $0x33,%ebx / jl`), so the missing one-time guard is in the shared source, not one port.
  ⚠ The obvious "guard" candidates in the arm64 disasm are decoys: `ldr w28,[x27,#8]` /
  `cmp w26,#0x33` are the loop itself.
- **Reproduction:** transcribed as-is with a `⚠` note in
  `src/game/servercore/CSWSEncounter/CSWSEncounter.cpp` (v620).

## `CSWSTrigger::LoadTrigger` — the OnClick script's GFF default is read from the OnEnter slot

- **Where:** `_ZN11CSWSTrigger11LoadTriggerEP7CResGFFP10CResStruct` @0x5aa74c. The seven script
  slots are read in one uniform run of
  `ReadFieldCResRef(pStruct, "<label>", bSuccess, CResRef(m_scriptSlots[i])).CopyToString(s);
   m_scriptSlots[i] = s;` — each slot defaulting to its own current value, so an absent GFF field
  leaves the slot alone. The seventh (`"OnClick"`, slot 6 @+0x3c8) breaks the pattern: its default
  is built from **`m_scriptSlots[1]`** (`ScriptOnEnter`, +0x378). The write-back still targets
  +0x3c8, so it is the default argument alone that is wrong — a copy-paste slip.
- **Impact:** a UTT with no "OnClick" field does not keep its existing OnClick script; it silently
  inherits whatever the OnEnter slot holds at that moment (which the same run has already
  overwritten from the GFF two calls earlier). Templates that omit OnClick therefore get an OnEnter
  script wired to the click event. `SaveTrigger` writes the two slots independently and correctly,
  so a load/save round-trip PERSISTS the corruption rather than reverting it.
- **Confirmed in both builds:** arm64 reuses the `add x22, x19, #0x378` register it materialized
  for the ScriptOnEnter read six calls earlier (`mov x1, x22` at 0x5aaa44) while computing
  `add x0, x19, #0x3c8` fresh for the write-back at 0x5aaa78 — the two offsets are visibly
  different in one basic block. The macOS oracle does the same through a spill slot
  (`movq -0x2e8(%rbp), %rsi`, loaded from `leaq 0x378(%r14)`). So the slip is in the shared source.
- **Reproduction:** transcribed as-is with a `⚠` note in
  `src/game/servercore/CSWSTrigger/CSWSTrigger.cpp` (v622).

## `CSWSPlaceable::LoadPlaceable` — the "GroundPile" GFF field is read, discarded, then forced to 1

- **Where:** `_ZN13CSWSPlaceable13LoadPlaceableEP7CResGFFP10CResStructi` @0x6a30fc. The read is
  spelled exactly like its correctly-working neighbours — `ReadFieldBYTE(pStruct, "GroundPile",
  bSuccess, m_bGroundPile)`, i.e. defaulting to the live value so an absent field leaves the flag
  alone — but the **result is never consumed**. The very next instruction stores a literal `1`
  into the field unconditionally.
- **Impact:** every placeable that goes through `LoadPlaceable` comes out with `m_bGroundPile == 1`
  regardless of what the blueprint or savegame said, so the flag cannot distinguish an auto-created
  dropped-loot pile from a hand-placed object. `SavePlaceable` writes the field out faithfully, so
  a load/save round-trip **rewrites every placeable's GroundPile to 1** and the corruption
  persists. Because the default argument is the *correct* one, this reads as a dropped assignment
  (`m_bGroundPile = ` lost from the front of the statement) rather than a deliberate override.
- **Confirmed in both builds:** arm64 `bl <ReadFieldBYTE>` at 0x6a3850 with w0 dead, then
  `mov w8, #1 / str w8, [x19, #0x370]` at 0x6a385c — while the *preceding* call at 0x6a3830 still
  loads `ldr w4, [x19, #0x370]` as this call's default argument. The macOS oracle has the identical
  shape (`callq ReadFieldBYTE` at 0x10039a7d9, then `movl $0x1, 0x370(%r15)`), so the slip is in
  the shared source, not a per-target optimization artifact.
- **Reproduction:** transcribed as-is with a `⚠` note in
  `src/game/servercore/CSWSPlaceable/CSWSPlaceable.cpp` (v623).


## `CSWSCombatAttackData::LoadData` — the "DamageList" read is unbounded

- **Where:** `_ZN20CSWSCombatAttackData8LoadDataEP7CResGFFP10CResStruct` @0x594c1c (macOS
  0x1003dda0c). The writer half, `SaveData` @0x594918, emits exactly 15 `DamageList` elements —
  its loop is a literal `for (i = 0; i < 15; i++)` over `m_anDamage[15]`. The reader's loop instead
  runs to **the GFF's own element count**: `for (i = 0; i < GetListCount(&damageList); i++)`, with
  the element index used directly as `m_anDamage[i]`.
- **Impact:** a save file (or a hand-edited/corrupted GFF) carrying more than 15 `DamageList`
  entries writes past the end of the 15-short array, straight into the fields that follow it —
  `m_nWeaponAttackType`/`m_nAttackMode`/`m_nConcealment` at +0x42..+0x44, then
  `m_bRangedAttack` at +0x48 and onward. Two shorts of overrun per extra element. Nothing in the
  loop clamps the index and nothing re-checks the count afterwards.
- **Confirmed in both builds:** arm64 re-reads `GetListCount` at the bottom of the loop
  (`bl <GetListCount>; cmp w?, w0; b.lo <top>`) with the raw index feeding
  `strh w0, [x19, w?, sxtw #1]` off +0x24; macOS is the same shape
  (`callq __ZN7CResGFF12GetListCountEP8CResList; cmpl %eax, %ebx; jb`, then
  `movw %ax, 0x24(%r15,%rcx,2)`). So the missing bound is in the shared source, not a per-target
  artifact.
- **Note:** in practice the engine only ever writes its own 15-element lists, so this is reachable
  only through a malformed/tampered save — but it is a straightforward unchecked-length write.
- **Reproduction:** transcribed as-is with a `⚠` note in
  `src/game/servercore/CSWSCombatAttackData/CSWSCombatAttackData.cpp` (v634).


## `CSWSCombatAttackData::Add{Melee,Ranged}SubAttack` — dead entry points that copy uninitialised stack

- **Where:** `_ZN20CSWSCombatAttackData17AddMeleeSubAttackEj` @0x595310 and
  `_ZN20CSWSCombatAttackData18AddRangedSubAttackEihji6Vectorji` @0x5957a0.
- **Dead:** a PLT-aware scan of every `bl`/`b` in `libkotor2.so`'s `.text` finds ZERO callers for
  either one, and neither exists in the macOS build at all. The live paths are
  `ResolveAttackToMeleeSubAttack` @0x5950c0 and `ResolveAttackToRangedSubAttack` @0x595530, which
  `CSWSCreature::Resolve{Melee,Ranged}Attack` call. These two look like the earlier hand-rolled
  entry points the Resolve* pair replaced; they survive only because they are exported.
- **Bug:** both build a `CSWSCombatSubAttack` local whose constructor initialises only the Vector
  and the five breakdown records, assign two (melee) or eight (ranged) head fields, and then hand
  the whole 0xf0-byte record to `CExoArrayList<CSWSCombatSubAttack>::Add` **by value**. The head
  slots neither the constructor nor the body writes are copied out of uninitialised stack: +0x00,
  +0x04, +0x06, +0x08 and +0x10 for the melee form, +0x06/+0x08 and +0x10 for the ranged one. The
  queued sub-attack would carry a garbage attack index, attack result, feat id and ready state.
- **Visible in the disasm, and the asymmetry is the proof it is the source and not the compiler:**
  slots that are pure `undef` (+0x10/+0x14 in the melee form) are DROPPED by clang, while the ones
  the by-value copy has to materialise survive as real reloads off the alloca. A body that
  initialised everything would show neither.
- **Reproduction:** transcribed as-is, with `⚠` notes on both functions, in
  `src/game/servercore/CSWSCombatAttackData/CSWSCombatAttackData.cpp` (v635).

## `CSWSCombatAttackData::ResolveAttackToMeleeSubAttack` — leaves three queue fields uninitialised

- **Where:** @0x5950c0 (macOS @0x1003dde76). Same class, live path, milder version of the above.
- **Bug:** the melee form never writes the sub-attack's +0x10, +0x14 or +0x2c, all three of which
  the ranged form fills from its extra parameters. The constructor does not touch them either, so
  the queued record carries stack garbage in the three slots `CSWSCreature::SignalRangedDamage`
  reads as "the projectile resolved" marker, the damage/effect fan-out gate, and the projectile
  type. In practice the melee path never reaches the ranged consumers, so the garbage is
  unobserved — but nothing enforces that.
- **Confirmed in both builds** (arm64 stores only +0x18/+0x00/+0x0c/+0x04/+0x08/+0x06; macOS the
  same set), so it is in the shared source.
- **Reproduction:** transcribed as-is in the same file (v635).

## `CSWSCreature::LoadCreature` — the saved "DetectMode" is read and thrown away

- **Where:** @0x62eae8 (macOS @0x10038a638). The whole-creature GFF reader, counterpart of
  `SaveCreature` @0x62f08c which faithfully WRITES the field (`WriteFieldBYTE(pStruct,
  m_detectMode, "DetectMode")`).
- **Bug:** the reader calls `ReadFieldBYTE(pStruct, "DetectMode", bSuccess, 0)` and then discards
  the result, passing a hardcoded `1` to `SetActivity`:
  `SetActivity(2, 1, 0)`. Every creature therefore loads with detect mode ON regardless of what
  was persisted. The very next statement is the same shape written CORRECTLY —
  `SetActivity(1, ReadFieldBYTE(..., "StealthMode", ...), 0)` — which is what makes this read as a
  copy-paste slip rather than an intentional force-on.
- **Confirmed in both builds:** arm64 sets bit 1 of the activity mask unconditionally (the inlined
  `SetActivity` body's `orr w9, w8, #2` with no reference to the value just read), and the macOS
  oracle shows the same — `movl $0x2,%esi; movl $0x1,%edx; xorl %ecx,%ecx` with `%al` dead. So it
  is in the shared source, not a per-target codegen artifact.
- **Reproduction:** transcribed as-is, with a `⚠⚠` note on the function, in
  `src/game/servercore/CSWSCreature/CSWSCreature.cpp` (v636).

## `CSWSCreature::AIActionCheckInterAreaPathfinding` — the rebuilt TRANSITION list is copied from the AREA list

- **Where:** @0x5410cc (macOS @0x10043ebac). The per-frame driver for an inter-area route.
- **Background:** `CPathfindInformation` holds the route as two parallel owned arrays —
  `m_paInterAreaIds` (+0xe0, the areas in order, `m_nInterAreaCount` of them) and
  `m_paInterAreaTransitionIds` (+0xe8, the trigger/door object between each adjacent pair, so one
  SHORTER). `CSWSModule::ComputeInterAreaPath` @0x5b0fe0 pins the relationship exactly: it sets the
  count to 10 and then allocates `0x50` (10 * 8) bytes for the first and `0x48` (9 * 8) for the
  second. `CSWSModule::SetIntraAreaGoal` @0x5b12b4 resolves `m_paInterAreaTransitionIds[0]` through
  `CGameObjectArray::GetGameObject` and requires it to be a TRIGGER or a DOOR.
- **Bug:** popping the head off the route rebuilds both arrays, and BOTH copy loops read
  `m_paInterAreaIds[i + 1]`. The new transition array is therefore filled with AREA object ids. The
  very next hop hands `m_paInterAreaTransitionIds[0]` to `SetIntraAreaGoal`, which looks that id up
  and finds a `CSWSArea`, not a trigger or a door — the object-type test fails and the function
  returns without ever setting an intra-area goal. So every inter-area route past its FIRST hop
  silently stops steering, and the creature falls back on whatever goal the previous hop left in
  `m_Unk074`. The obvious intent was `m_paInterAreaTransitionIds[i + 1]`.
- **Confirmed in both builds:** the arm64 loops at 0x5411f4 and 0x54128c both load through a base
  taken from `[x23, #0xe0]`, and the macOS pair at 0x10043ec87 / 0x10043ecb1 both use `0xe0(%rbx)`.
  Neither build so much as reads +0xe8 before the two `delete[]`s. Shared source, not codegen.
- **Reproduction:** transcribed as-is, with a `⚠⚠` note at the marker, in
  `src/game/servercore/CSWSCreatureAI/CSWSCreatureAI.cpp` (v646).

## `CSWSCreature::EquipMostDamagingMeleeWeapon` — the incumbent weapon's score is computed and thrown away

- **Where:** @0x63a58c (macOS @0x10039440a). "Which melee weapon in my pack hits this target
  hardest?", answered by brute force: strip the weapon hands into the repository, equip each
  candidate in turn, score it, unequip, then re-queue the winner via `AddEquipItemActions`.
- **Background:** the RANGED twin `EquipMostDamagingRangedWeapon` @0x63abc4 (MATCHED, v649) is the
  same algorithm and gets this right. It scores the weapon the creature is already holding FIRST
  and seeds the running best with it (`nBestModifier = m_pStats->GetAttackModifierVersus(pTarget)`),
  so the incumbent is the score to beat; `-30` is only the "nothing scored yet" floor.
- **Bug:** the melee version performs the identical baseline measurement — it calls
  `m_pStats->GetDamageRoll(pTargetObject, 0, 0, 0, 1, 1)` and then
  `CalculateMaxElementalDamage(pTarget, 0)` immediately before the loop — and **discards both
  results**. The running best is initialised to plain `0` instead. Since a damage roll is never
  negative, ANY candidate in the repository that rolls `>= 0` ties or beats the weapon the creature
  is actually holding, and the loop's `>=` comparison means a tie also wins. The practical effect is
  that a creature asked to optimise its melee weapon will happily swap its best sword for the first
  junk vibroblade in its pack, and pays a real equip action to do it. The obvious intent was
  `nBestDamage = GetDamageRoll(...) + CalculateMaxElementalDamage(...)`, exactly as the ranged twin
  does with its own metric.
- **Confirmed in both builds:** arm64 calls both at 0x63a8ac / 0x63a8c0 and then immediately does
  `str wzr, [sp, #0x3c]` (the running best) at 0x63a8cc — the returned `w0` is never read. macOS
  calls them at 0x10039465b / 0x100394669 and then `xorl %eax,%eax; movq %rax,-0x78(%rbp)` at
  0x100394694. Both builds keep the calls only because they are opaque and side-effecting
  (`GetDamageRoll` writes the attack data's +0x140..+0x145 bytes and the combat log); only the
  arithmetic is dead. Shared source, not codegen.
- **Reproduction:** transcribed as-is, with a `⚠⚠` note at the marker, in
  `src/game/servercore/CSWSCreature/CSWSCreature.cpp` (v650).

## `CSWSCreature::SavingThrowRoll` @0x672f94 — the queued-breakdown arm skips the null check its own siblings make (v652)

- **What the function does:** rolls a saving throw, builds a `CSWCCMessageData` breakdown, and then
  ships that breakdown one of two ways depending on its seventh parameter (`bQueueToAttack`):
  either by broadcasting it to the saver and the source creature, or by APPENDING it to the source
  creature's current combat-round attack record.
- **Bug:** the appending arm does
  `pSource->m_pCombatRound->GetAttack(pSource->m_pCombatRound->m_currentAttack)->m_messages.Add(pData)`
  with **no null check on `pSource`**, even though `pSource` is nothing more than the result of
  `pObject->AsSWSCreature()` on an object resolved from a caller-supplied object id — and even
  though EVERY other use of `pSource` in the same function is guarded (`if (pSource != 0 && …)` on
  the broadcast path, `if (pSource != 0)` around the record write immediately after). A saving
  throw queued against a source id that resolves to a non-creature (a placeable, a trigger, a
  door, an area-of-effect object) or to nothing at all faults on the `m_pCombatRound` load.
- **Why it is latent rather than fatal:** the only transcribed caller that passes
  `bQueueToAttack == 1` is `CSWSCreature::ResolveMeleeAttack` @0x6710a4's unarmed force-push block,
  which reaches the call through `pTarget->AsSWSCreature()` and so happens to always supply a
  creature. The guard is missing, not the invariant.
- **Confirmed in both builds:** arm64 `0x673318: ldr x8, [sp, #0x48] / 0x67331c: ldr x0, [x8,
  #0x16a0]` with the preceding `cbz w27` testing only the flag; macOS `0x1003457f1: movq
  -0x40(%rbp), %rax / 0x1003457f5: leaq 0x16a0(%rax), %r13` likewise. Shared source, not codegen.
- **Reproduction:** transcribed as-is, with a `⚠` note at the marker, in
  `src/game/servercore/CSWSCreatureCombat/CSWSCreatureCombat.cpp` (v652).

## `CSWSCreature::ResolveOnHitEffect` @0x67718c — on-hit chance/duration LEAK BETWEEN PROPERTIES (v657)

- **Where:** the passive on-hit item-property pass, `src/game/servercore/CSWSCreatureCombat/
  CSWSCreatureCombat.cpp`.
- **Shape:** the function walks every passive property on the attacking weapon and, for each
  itempropdef **type-0x20** ("On Hit: …") property, looks the effect's numbers up in the 2DAs
  before rolling for it. The two numbers live in `nChance` and `nDurationRounds`.
- **Bug:** those two locals are **function-scope and initialised exactly once, ahead of the loop**
  (a single merged `str xzr,[sp,#0x40]` covering both), not per property. They are only refreshed
  when `IPRP_ONHIT`'s `Param1ResRef` column reads **1** for that subtype. So a type-0x20 property
  whose `Param1ResRef` is not 1, and whose switch arm does not hardcode a chance, **inherits the
  chance and duration of whichever property was processed before it on the same weapon**.
- **Who is exposed:** the two arms that neither hardcode `nChance = 100` nor come from a
  parameterised row — subtypes **8** (`ApplyOnHitSlayRacialGroup`) and **9**
  (`ApplyOnHitSlayAlignmentGroup`). On the first property of a weapon they inherit the zero-init
  and simply never fire; behind a parameterised property they inherit its numbers.
- **Confirmed in both builds:** arm64 zeroes both slots once at `0x677200` (`str xzr,[sp,#0x40]`,
  covering `sp+0x40` and `sp+0x44`) with the loop entered at `0x67721c`; the macOS oracle does the
  same with two `movl $0x0` stores at `0x1003490eb`/`0x1003490f2`, likewise **before** the loop.
  Shared source, not codegen.
- **Reproduction:** transcribed as-is, with a `⚠` note at the marker. **Do not "fix" it by moving
  the declarations inside the loop** — that changes the frame and the zero-init shape.

## `CSWSCreature::DoSpotDetection` @0x63220c — the DISTANCE term has the WRONG SIGN (v659)

- **Where:** the visual half of the opposed stealth check, `src/game/servercore/CSWSCreature/
  CSWSCreature.cpp`. Its audio twin is `DoListenDetection` @0x631c78.
- **Shape:** the function accumulates a detection score (`> 0` means spotted) out of the spotter's
  Spot roll and rank minus the hider's Hide roll and rank, then applies situational modifiers.
  The last modifier is range: `fSteps = magnitude(self - target) / 3.0f`, and for `fSteps > 2.0f`
  it derives `nSteps = (int)(fSteps - 2.0f)`.
- **Bug:** the reported modifier is stored **already negated** (`nDistancePenalty = -nSteps`) and
  the score is then updated with **`nDetect -= nDistancePenalty`**, i.e. the negation is applied
  twice. **Distance is ADDED to the detection score, so a creature gets EASIER to spot the further
  away it is** — the opposite of the intent, and the opposite of what the modifier reported to the
  client says.
- **The twin proves the intent:** `DoListenDetection` computes the same quantity and applies it as
  a plain `nDetect -= nSteps` (macOS `subl %eax,%ebx` @0x10038d9d8). Only the Spot path is wrong.
- **Confirmed in both builds:** arm64 emits `neg w23,w8` @0x6325fc (the reported value) next to
  `add w26,w26,w8` @0x632600 (the score), so the two really do disagree in sign; the macOS oracle
  does the same with `addl %eax,%r15d` then `negl %eax` @0x10038deca. Shared source, not codegen.
- **Effect in play:** the range term is small (one point per 3 world units beyond 6), but it is
  *unbounded upward*, so at long range it dominates and stealth degrades with distance. The
  early-out on `GetSpotCheckDistance()` caps how far this can be reached — except that the cap is
  itself bypassed whenever the target has `m_bForceAlwaysUpdate` set.
- **Reproduction:** transcribed as-is, with a `⚠` note at the marker. **Do not "fix" it to `+=`.**

## `CSWSCreature::AIActionItemCastSpell` @0x53f034 — the attack-of-opportunity guard tests the SAME base-item type TWICE (v673)

- **Where:** the use-item-property executor (action id 0x2e), `src/game/servercore/CSWSCreatureAI/
  CSWSCreatureAI.cpp`. The guard sits in the once-per-action `m_Unk1634 == 0` block that decides
  whether activating an item provokes an attack of opportunity.
- **Shape:** the row contains the same "is this a thrown/placed item" idiom THREE times. The two
  later copies (in both textual copies of the cast-context block) test the trio
  `GetBaseItem()->m_Unk0xd8 == 0x19 || == 0x2d || == 0x2f`. The attack-of-opportunity copy tests
  only two disjuncts — and **both of them are `== 0x19`**.
- **Bug:** the second disjunct is dead, so base-item types **0x2d and 0x2f never broadcast the
  attack of opportunity** even though the surrounding code treats all three types alike everywhere
  else. The duplicated compare also costs a redundant `CSWItem::GetBaseItem()` call, which is why
  it is visible in `bltally` as 13 calls rather than 12.
- **Confirmed in both builds:** arm64 emits `bl GetBaseItem / ldrb w8,[x0,#0xd8] / cmp w8,#0x19 /
  b.eq` immediately followed by a byte-identical second copy @0x53f7ac..0x53f7d0; the macOS oracle
  does exactly the same at 0x10043cd8c..0x10043cdba (`cmpl $0x19` twice). Shared source, not
  codegen — a copy-paste that lost its edit.
- **The trio proves the intent:** the same guard 0x80 bytes later reads
  `== 0x19 || == 0x2d || == 0x2f`, three calls and three distinct constants.
- **Reproduction:** transcribed as-is, with a `⚠` note at the marker. **Do not "fix" the second
  disjunct to `== 0x2d`.**

## `CSWSCreature::AddItemCastSpellActions` @0x61fa8c — the grenade/item spell id is read UNINITIALIZED when the item has no active property (v674)

- **Where:** the QUEUER half of the item-cast-spell pair (the consumer is
  `AIActionItemCastSpell` @0x53f034), `src/game/servercore/CSWSCreature/CSWSCreature.cpp`.
- **Shape:** the row calls `pItem->GetActiveProperty(nProperty)` and, **only if the result is
  non-null**, reads the property's `m_subType` into a local. That local is then passed
  unconditionally to `ConvertGrenadeSpellID`, whose result indexes `CSWSpellArray::GetSpell`.
- **Bug:** on the null-property edge nothing ever assigns the local, so the spell id handed to
  `ConvertGrenadeSpellID` is whatever `GetActiveProperty` happened to leave in the argument
  register. In practice the subsequent `GetSpell(...) == 0` test usually rejects it, but the
  behaviour is unspecified and a colliding value would queue the WRONG spell.
- **Confirmed in both builds:** arm64 sets `mov w1, w21` (the nProperty argument) before
  `bl GetActiveProperty`, and on the `cbz x0` edge branches straight to `bl ConvertGrenadeSpellID`
  with `w1` never re-written; the macOS oracle does the identical thing with `%esi`
  (`movl -0x44(%rbp),%esi` / `callq GetActiveProperty` / `je 0x10037bcf3` / `callq
  ConvertGrenadeSpellID`). Shared source, not codegen.
- **The consumer proves the intent:** `AIActionItemCastSpell` @0x53f034 runs the same
  `GetActiveProperty` call and **bails out of the action** when it returns null. The queuer simply
  forgot the guard.
- **Reproduction:** transcribed as-is with a deliberately uninitialized `int nSubType;` and a `⚠`
  note at the marker. **Do not "fix" it by adding an early return or a zero initialiser** — either
  one changes the emitted code.

## `CSWVirtualMachineCommands::ExecuteCommandGetAttacker` — GetFirstAttacker/GetNextAttacker DISCARD their results (v936)

- **Where:** `ExecuteCommandGetAttacker` @0x575378, the shared handler for NWScript commands 727
  (GetFirstAttacker) and 728 (GetNextAttacker). Both arms call the accessor
  (`bl CSWSCreature::GetFirstAttacker` @0x5753fc / `bl …GetNextAttacker` @0x57540c) and then
  branch to a merge block that pushes **`x20`**, which was loaded from
  `CGameObjectArray::INVALIDOBJID` at 0x5753e0 and is never written again. `x20` is callee-saved,
  so the accessor cannot have changed it — the return value in `x0` is simply dropped.
- **Impact:** both script commands **always return OBJECT_INVALID**, regardless of the creature's
  recent-attackers list. Any module script that walks attackers with them sees an empty list. The
  underlying `CSWSCreature::GetNextAttacker` cursor (`m_nNextAttackerIdx` +0x17b0) still ADVANCES
  on every call, so repeated calls silently consume the list while reporting nothing.
- **Both builds agree**: the macOS oracle does the same — `callq __ZN12CSWSCreature16GetFirstAttackerEv`
  at 0x100312493, then `leaq __ZN16CGameObjectArray12INVALIDOBJIDE(%rip)` / `movq (%rax),%rsi`
  at 0x100312498 in the merge block. So it is source: the author forgot the `oidAttacker =`.
- **Reproduction:** the two bare `pCreature->GetFirstAttacker();` / `GetNextAttacker();` statements
  in `src/game/servercore/CSWVirtualMachineCommands/CSWVirtualMachineCommands.cpp`, marked `// sic`.
  Do NOT add the assignment — it changes the push argument and the whole tail.
- ⭐ **The generalisable check:** when a command's push argument comes from a CALLEE-SAVED register
  that was loaded before the branchy middle, verify that some arm actually writes it. A `bl` whose
  `x0` is never read is invisible to bltally (call parity is fine) and to constaudit.

## `CSWVirtualMachineCommands::ExecuteCommandDistanceConversions` — the result float is UNINITIALIZED on the fall-through arm (v936)

- **Where:** `ExecuteCommandDistanceConversions` @0x571064, the shared handler for FeetToMeters
  (218) and YardsToMeters (219). Each arm loads the popped float and multiplies by its factor
  (0.3048f / 0.9144f); the implicit `else` reaches `StackPushFloat` with `s0` never written.
- **Impact:** any other command id routed to this handler pushes whatever `s0` happens to hold.
  Not reachable through the shipped 877-entry table, which only maps 218/219 here — a latent
  hazard rather than a live bug.
- **Both builds agree** (macOS @0x10030ecfb falls into the shared `callq …StackPushFloat` with
  `%xmm0` untouched on the miss edge). Transcribed verbatim; the `float fResult;` declaration is
  deliberately uninitialized. Same class as the v935 object-state getters above.

## `CSWVirtualMachineCommands` — two commands report a PUSH failure for a POP failure (v936)

- **Where:** `ExecuteCommandGivePlotXP` @0x584440 returns **-2000** when `StackPopString` fails
  (its integer pop correctly returns -2001), and `ExecuteCommandSetGlobalString` @0x589b48 returns
  -2000 for BOTH of its string pops. Every one of the ~370 other commands uses -2001 for a pop
  failure and -2000 only for a failed push.
- **Impact:** cosmetic — the VM logs/propagates the wrong error code for a malformed call to those
  two commands. Both builds agree, so it is source (a copy-paste of the push arm's constant).
- **Reproduction:** the `return -2000;` after the string pops in those two bodies, marked `// sic`.
- ⭐ **v937 raises the count to FIVE.** `ExecuteCommandGetGlobalString` @0x577584 and
  `ExecuteCommandGetGlobalLocation` @0x5774a0 return -2000 for their name-string pop (the same
  copy-paste as SetGlobalString, which is their writer twin), and
  `ExecuteCommandAddBonusForcePoints` @0x58dc14 returns -2000 for BOTH pops — including its
  `StackPopObject`, so the slip is not confined to string pops. The global-variable family is now
  4-for-4 on this bug: every command that touches `CSWGlobalVariableTable` gets the code wrong.
- ⭐ **v938 raises the count to SEVEN, and the global-variable family to 5-for-5.**
  `ExecuteCommandSetGlobalLocation` @0x589a54 returns -2000 for its name-string pop but the
  correct -2001 for the engine-structure pop that follows it — the two codes side by side in one
  body, which pins the slip as a per-POP copy-paste rather than a per-command habit. And
  `ExecuteCommandSetBonusForcePoints` @0x58db18 returns -2000 for BOTH pops, exactly like its
  AddBonusForcePoints sibling: the whole bonus-force-point pair gets it wrong.

## `CSWVirtualMachineCommands::ExecuteCommandSWMG_GetMiniGameObjects` @0x64cc84 — the shared tail dereferences an UNINITIALIZED pointer on the impossible third arm (v937)

- **Where:** the handler serves SWMG_GetEnemy (0x265) and SWMG_GetObstacle (0x267) from one body.
  Each arm assigns the lookup result to a local, and both fall into a shared
  `pObject->m_nObjectId` load that clang tail-merged (`0x64ccec b.ne #0x64cd00` jumps INTO the
  shared `ldr w1,[x0,#0xc]` without ever setting x0). The `else if` chain has no final `else`, so
  a command id other than those two reaches that load with x0 holding `StackPopInteger`'s return
  value.
- **Impact:** none in practice — only 0x265 and 0x267 are bound to this handler in
  `InitializeCommands`, so the third edge is unreachable. It is the same class as
  `AIActionExamineMine` above (a value assigned in every explicit arm of a chain whose implicit
  final `else` still reaches the consumer), and it is transcribed verbatim: `pObject` is
  deliberately left uninitialized. Adding `= 0` costs bytes and changes the CFG.

## `CSWVirtualMachineCommands::ExecuteCommandSWMG_GetLastEvent` @0x64be70 — answers the object's NAME, not any event (v938)

- **Where:** the body (command 583) looks OBJECT_SELF up in the mini-game object array and, when
  it resolves, copies `pObject->m_sName` (+0x18) into the string it pushes. `m_sName` is the
  display name the array's own `GetMiniGameObjectByName` matches against — there is no event
  record anywhere in the body, and no other field is read.
- **Impact:** live and user-visible to scripts: `SWMG_GetLastEvent()` returns the calling object's
  name instead of whatever event string it was meant to report. Every mini-game script branching
  on the result compares against a name.
- **Reproduction:** the `sEvent = pObject->m_sName;` assignment in the transcribed body. The push
  then makes the same `CStr()` round-trip through a second `CExoString` that `ObjectToString`
  does, so the wrong value is even copied twice on the way out.

## `CSWVirtualMachineCommands::ExecuteCommandChangeItemCost` — leaks a probe item on every call, and writes the SHARED baseitems.2da row

- **Where:** @0x568a80. Two independent bugs in six lines. (1) The command builds a probe item with
  `new CSWSItem(0x7f000000)`, loads the caller's template into it — and never deletes it. Every
  successful *and* every failing call leaks 0x408 bytes plus whatever `LoadFromTemplate` allocated;
  the only `operator delete` in the function is the ctor-throw landing pad. (2) The write lands on
  `pItem->GetBaseItem()->m_costMultiplier`, i.e. the shared `CSWBaseItem` row the whole session
  indexes by base-item type — so "change the cost of THIS item" silently re-prices **every item of
  that base type**, permanently, and the probe item it was aimed at is thrown away immediately.
- **Impact:** an unbounded leak on a script-callable command, and a global economy side effect no
  script author would expect. Both builds identical (the macOS oracle's tail is the same
  `LoadFromTemplate` / `GetBaseItem` / store with no delete).
- Transcribed verbatim; the body comment carries the same warning.

## `CSWVirtualMachineCommands::ExecuteCommandGetCreatureHasTalent` — leaks the talent structure when the push fails

- **Where:** @0x572d28. `delete pTalent` sits AFTER the `StackPushInteger` success test, so the
  `-2000` (push failed) return path drops the engine structure the command owned. The sibling
  `GetIsTalentValid` @0x57a5d0 deletes BEFORE the push and does not have this. Same one-line
  ordering slip as the other "consumes the structure" commands, but only on the error edge.

## The −2000/−2001 pop-failure transposition is now **NINE** commands, and the global-variable family is **7-for-7**

- v938 recorded seven. v939 adds `ExecuteCommandSetGlobalNumber` @0x589950 (BOTH pops — string and
  int — answer `-2000`, the push code) and `ExecuteCommandGetGlobalNumber` @0x577380 (its string pop
  likewise). With `SetGlobalLocation`, `GetGlobalLocation`, `SetGlobalString`, `SetBonusForcePoints`
  and `AddBonusForcePoints` already recorded, **every transcribed member of the global-variable
  command family gets its pop-failure code wrong**, which pins it as a copy-paste of one bad
  skeleton rather than nine independent slips. `ExecuteCommandSetSubType` @0x570e08 does the same
  outside that family.

## `CSWVirtualMachineCommands::ExecuteCommandGetLastAttackMode` — pushes the OBJECT_SELF bit pattern as an INTEGER

- **Where:** @0x57b6fc. A missing object, a non-creature, and a creature that is not in combat
  (`m_Unk0x728 == 0`) all take `mov w1, #0x7f000000` into `StackPushInteger`. `0x7f000000` is the
  OBJECT_SELF/invalid-OBJECT sentinel, not a `COMBAT_MODE_*` constant — script-visible, and no
  nwscript constant compares equal to it, so `GetLastAttackMode()` outside combat can only be
  detected by testing for the raw magic number.

## `CSWVirtualMachineCommands::ExecuteCommandGetIsRacialTypePlayable` — indexes racialtypes.2da ONE PAST THE END

- **Where:** @0x57a4c0. The non-creature and missing-object arms set the index to
  `g_pRules->m_nNumRaces` — the ROW COUNT — and then unconditionally read
  `m_paRaces[nRace].m_bPlayerRace`. That is a read of the first 4 bytes past the
  `new CSWRace[m_nNumRaces]` allocation, so the answer is whatever the heap holds there. Not a
  crash in practice (the block is followed by allocator metadata) but the result is undefined.

## `CSWVirtualMachineCommands::ExecuteCommandTakeGoldFromCreature` — dereferences the OBJECT_SELF lookup with no null check

- **Where:** @0x58dda0. The taker is resolved as
  `GetGameObject(m_oidSelf)->AsSWSObject()` and only the DOWNCAST result is null-tested — the
  `ldr x8,[x0]` that loads the vptr for the virtual call runs on the raw lookup result. Any script
  whose OBJECT_SELF has already been destroyed (the command is reachable from a delayed action)
  crashes here rather than answering 0. The macOS build has the same shape.

## `CSWVirtualMachineCommands::ExecuteCommandActionJumpToPoint` — reads the area's object id BEFORE null-checking the area

- **Where:** @0x5632c8. `GetArea()`'s result is converted to its CGameObject base and
  `m_objectId` is loaded unconditionally; the `cbz` that rejects a null area comes AFTER. On arm64
  the MI adjust (`csel x8,xzr,x8,eq`) makes the faulting address literally `8`, and the **macOS
  oracle proves it is in the SOURCE, not a codegen artifact**: it constant-folds the null arm into
  `movq 0x8, %rax` — an absolute load from address 8 — before jumping to the shared exit. A
  creature not yet attached to an area (mid-transition, or a bare template object) crashes.

## `CSWVirtualMachineCommands::ExecuteCommandSetPartyLeader` — the party-member resurrect loop re-tests a flag it already required

- **Where:** @0x58b254, the `nNPC == -1` arm. The loop only reaches the revive block after
  `m_bInParty == 1 && m_Unk16b4 == 1`, yet the revive condition then asks
  `GetDead() == 1 || (m_bInParty != 0 && GetCurrentHitPoints(0) <= 0)`. The second `m_bInParty`
  test can never be false. Harmless, but it pins the revive condition as a COPY of
  CServerExoAppInternal::TempDeathUpdate's (where the flag really is unknown) rather than
  something written for this site.

## `CSWVirtualMachineCommands::ExecuteCommandSetJournalQuestEntryPicture` — answers 1, not 0, when there is no player list

- **Where:** @0x58a328. Every other exit — a failed lookup, an empty party, a successful fan-out —
  returns 0; only `GetPlayerList() == NULL` returns 1. The VM treats any nonzero return as a
  command-level failure code, so a null player list reports a bogus status instead of a no-op.

## `CSWVirtualMachineCommands::ExecuteCommandActionMoveAwayFromLocation` — leaks the popped CScriptLocation on every SUCCESSFUL call

- **Where:** @0x56386c. Only the argument-pop failure arm runs `delete pLocation` (and null it).
  Both the "already outside the radius" early-out and the path that actually queues the walk
  return 0 with the heap-allocated `CScriptLocation` still live, so every in-range
  `ActionMoveAwayFromLocation` leaks 0x18 bytes. Its own sibling `ExecuteCommandMoveToPoint`
  @0x562b94 deletes on all paths, which is what makes this one visibly wrong rather than a
  convention. Same shape on both targets.

## `CSWVirtualMachineCommands::ExecuteCommandMoveToObject` — records the RANGE twice and never records the timeout

- **Where:** @0x562db0. Action 17's parameter list hands `&fRange` to BOTH the third and the
  fourth `(type 2, float)` slots; `&fTimeout` is never passed at all. So a
  `ActionForceMoveToObject`'s 30-second timeout is replaced by its range and never reaches the AI.
  Both targets emit the duplicated address from ONE address computation (x86
  `leaq -0x54(%rbp),%rax` then `movq %rax,0x20(%rsp)` **and** `movq %rax,0x10(%rsp)`), so it is
  the source, not a CSE artifact.

## `CSWVirtualMachineCommands::ExecuteCommandMoveToObject` — asks OBJECT_SELF whether the TARGET is a placeable

- **Where:** @0x562db0, the trailing arm. After the handler has already established
  `pObject->AsSWSCreature() != 0` on OBJECT_SELF, it asks `pObject->AsSWSPlaceable()` — on the
  same object. A creature is never a placeable, so the guard is unconditionally false and the
  follow-up action 0x13 is dead code. The obvious intent was `pTarget->AsSWSPlaceable()` (move to
  the placeable, then interact with it). Reproduced on both targets.

## `CSWVirtualMachineCommands::ExecuteCommandVersusEffect` — the sanctuary/invisibility arm drops the law/chaos half of a versus-alignment restriction

- **Where:** @0x58ea58. For command 355 (`VersusAlignmentEffect`) every effect family writes BOTH
  alignment components into the effect's integer block — concealment and immunity write index 2
  (nLawChaos) and index 3 (nGoodEvil); attack/damage/AC/skill write 3 and 4; saving throws 4 and 5.
  The sanctuary/invisibility arm writes ONLY index 3. Both targets branch that arm straight into
  concealment's *second* SetInteger (arm64 `b #0x58ec64`, x86 `je 0x100327523`), skipping the
  first, so the statement is missing from the source rather than folded away. A
  `VersusAlignmentEffect(EffectInvisibility(...), LAWFUL, GOOD)` therefore silently restricts on
  good/evil only.

## `CSWVirtualMachineCommands::ExecuteCommandEffectComplex` — leaks the CGameEffect on every failure path

- **Where:** @0x56779c. The effect is `new`ed BEFORE the switch, so the six argument-pop failure
  arms (EffectDeath's three optional pops, EffectRegenerate's two, EffectAssuredDeflection,
  EffectForceResisted, and both EffectDispelMagic*) all `return -2001` with the 0xe8-byte effect
  still live. So does the `StackPushEngineStructure` failure arm (`return -2000`). Only the
  success path reaches `delete pEffect`. Both targets: the arm64 failure label 0x567c38 goes
  straight to the canary check with no `bl operator delete` in between, and the macOS twin's
  0x1003058c8-equivalent does the same. A script that calls `EffectDeath()` with a malformed
  stack leaks 232 bytes per call.

## `CSWVirtualMachineCommands::ExecuteCommandSpawnMine` — spawns the mine in the MODULE's current area, not the creator's

- **Where:** @0x58d4bc. The trigger's area comes from `GetModule()->GetArea()`, i.e. whatever area
  the module currently has loaded, while the creator object is looked up independently by the
  popped object id. A script that spawns a mine on behalf of an off-screen creature therefore
  drops it wherever the player is standing. Neither port null-checks that area, nor the popped
  `CScriptLocation` — `SetPosition`, `CreateNewGeometry` and `AddToArea` all dereference them —
  so the only arm that can see a null location is the argument-pop failure arm, which is also the
  only one that deletes it. Reproduced on both targets.

## `CSWVirtualMachineCommands::ExecuteCommandRebuildPartyTable` — an unavailable companion keeps its stale object id

- **Where:** @0x586eb0. Each of the thirteen tag lookups only writes the slot when BOTH the tag
  resolves AND `GetIsNPCAvailable(slot)` is true; an unavailable companion is skipped rather than
  cleared, so whatever id the save wrote stays in the table. The tag→slot map is hard-coded here
  (not read from any 2DA) and is not in slot order — "mand" (Mandalore) is slot 2 and "visasmarr"
  is slot 9 — so a mis-tagged companion creature silently never rebinds. Both targets.

## `CSWVirtualMachineCommands::ExecuteCommandDoTouchAttack` — a DOOR or PLACEABLE target is an automatic hit with no roll

- **Where:** @0x58e2c0. When the target is not a creature the handler skips the whole attack
  resolution and returns 1 for a door or placeable and 0 for anything else — no d20, no AC, no
  attack bonus, and no feedback values, yet the feedback message is still built and sent with an
  UNINITIALISED attack bonus and roll (the two locals are only written on the creature path).
  Both targets read those two slots unconditionally in the message block. A touch attack against
  a door therefore reports whatever the previous stack contents were.

## `CSWVirtualMachineCommands::ExecuteCommandCreateObject` — a successfully created WAYPOINT is returned as OBJECT_INVALID

- **Where:** @0x569294. Four of the five `switch (nObjectType)` arms end by copying the new
  object's id into the return slot (`ldr x21,[x20,#8]` for store/placeable, `[x20,#0x20]` for the
  item's CSWSObject base at +0x18, `[x22,#8]` for the creature). The `OBJECT_TYPE_WAYPOINT`
  (0x20) arm branches straight to the push at 0x569510 with `x21` still holding
  `CGameObjectArray::INVALIDOBJID`, so `CreateObject(OBJECT_TYPE_WAYPOINT, …)` pushes
  OBJECT_INVALID even though the waypoint was constructed, loaded from its blueprint, positioned
  and added to the area. A script cannot get a handle on a waypoint it just created. Both
  targets — the macOS oracle's arm (`jmp 0x100307ec7` at 0x10030810f) skips the `%rbx` write the
  same way.

## `CSWVirtualMachineCommands::ExecuteCommandActionStartConversation` — script arguments 4 and 5 are popped and discarded

- **Where:** @0x564ee0. `nConversationType` (argument 4, popped into `[x29,-0x7c]` at 0x564fa0) is
  never read again — the action pack forwards `[x29,-0x78]`, a DIFFERENT local that is seeded to 1
  by the same `stp w8,w8,[x29,#-0x7c]` and never written afterwards. `bIgnoreStartRange`
  (argument 5, popped into `[x29,-0x80]` at 0x564fb8) is overwritten with a hard 1 at 0x564fc4,
  four instructions after the pop and on every path, before it reaches the pack. So a script
  asking for a non-cinematic conversation, or one that should respect the start range, gets the
  hard-coded behaviour either way. The pops themselves still have to happen — they consume the VM
  stack — which is presumably why the dead reads were never noticed.

## `CExoEncapsulatedFile::LoadHeader` — the ERF file object is deleted but not nulled

- **Where:** @0x7b660c. The success path sets `m_bHeaderLoaded = 1` and then runs the two-call
  `delete m_pFile` shape (`bl ~CExoFile` + `bl operator delete`) at 0x7b6960, and branches to the
  shared exit at 0x7b664c — skipping the `str xzr,[x19,#0x20]` that every FAILURE path executes.
  So a successfully-parsed archive is left holding a dangling `m_pFile`. `m_bFileOpened` was never
  set either, so `CloseFile` @0x7b604c early-outs and never touches it; the next `OpenFile`
  overwrites the pointer, which is what keeps this from crashing in practice.

## `CExoResourceImageFile::ReadResource` — the key list is indexed as an array of pointers

- **Where:** @0x7b6f84. Every other reader of the resource-image key list
  (`GetKeyList` @0x7b6d4c, `GetKeyListEntry` @0x7b41ec, `GetResourceSize` @0x7b6d5c,
  `GetResource` @0x7b6d78) treats `header + m_nEntriesOffset` as an array of INLINE 0x20-byte
  records and scales the index by 0x20. This one does `ldr x9,[x10, w9, uxtw #3]` — an 8-byte
  stride and then a DEREFERENCE — before reading `+0x18`/`+0x1c` off the result. The macOS build
  has the identical `movq (%rdi,%rsi,8),%rdi` at 0x1002dea3c, so it is the source, not a
  decompilation artifact: one of the two readings of the same table is wrong.

## `CExoResourceImageFile::OpenFile` — the second-extension retry leaks its CExoFile

- **Where:** @0x7b6dbc. When the `0xbc1` open fails the function deletes that object, nulls
  `m_pFile` and retries with `0xbba`. If the retry's `FileOpened()` returns 0 it branches straight
  to the epilogue at 0x7b6f50 with neither a `delete` nor a `str xzr,[x19,#0x20]` — the first
  failure path frees its object, the second does not.

## `CRes::CRes(unsigned int)` leaves three fields uninitialised that its sibling ctor zeroes

- **Where:** @0x7c07f4 vs @0x7c07c8. The default ctor's store range covers the whole tail
  (`stur q1,[x0,#0x18]` + `stur q1,[x0,#0x28]` = +0x18..+0x37, every pointer/size/flag). The
  resID-taking overload stops at `str wzr,[x0,#0x28]` — 4 bytes — so `m_nHolderCount` (+0x2c),
  `m_bOffsetAlloc10` (+0x30) and `m_bAllocSlack14` (+0x34) keep whatever the allocator left there.
  The last two are exactly the two flags `CExoResMan::Malloc` branches on when sizing the file-data
  allocation, so a CRes built through this overload can get a +10 or +14 slack allocation (or an
  offset `m_pFileData`) at random.

## `CRes::SetID` dereferences a null `this` its own inlined guard just checked for

- **Where:** @0x7c10bc. The body is `Dump(0); m_dwResID = dwResID;`. `CRes::Dump`'s first act is
  `if (this == 0) return;`, which inlines here as a `cbz x0` skipping the dump — and then the
  function falls into an UNCONDITIONAL `str w19,[x20,#0xc]` on that same null pointer. The guard
  buys nothing.

## `CExoResMan::ServiceFromImageRaw` asks for the resource size and throws the answer away

- **Where:** @0x7c5910. `blr` through vtable slot `GetResourceSize` at 0x7c59ac, then
  `mov w8,w19` / `str w19,[x21,#0x28]` — the container's answer never reaches a register that is
  used. Its BIF/KEY twin `ServiceFromResFileRaw` @0x7c5a78 does the clamp the call was for
  (`cmp w0,w21; csel w21,w0,w21,lt`). So a raw read from a resource-image pack trusts the caller's
  length with no bound at all. The Encapsulated twin does not even make the call.

## `CExoResMan::ServiceFromEncapsulatedRaw` Frees the resource it was asked to peek at

- **Where:** @0x7c57e8, at 0x7c58c0 — `Free(pRes)` on the SUCCESS path, between the read and the
  container's `DeleteRefCount`. None of the other three Raw variants does this; the whole point of
  the ReadRaw family is to read into a caller buffer without disturbing the CRes cache.

## `CExoResMan::Release` dereferences `GetTable`'s result with no null check

- **Where:** @0x7c0e3c. For a resource-image resource (`m_dwResID >> 30 == 1`) the function does
  `GetTable(pRes)->m_paDependentFiles[0]->DeleteRefCount()`. GetTable @0x7c2730 returns NULL for an
  empty list or an unmatched routing tag — its `mov x0,xzr` path is inlined here at 0x7c0ef8 and
  falls straight into `ldr x8,[x0,#0x40]`.

## `CExoResMan::Demand`'s async-in-flight early-out skips the demand-count bump

- **Where:** @0x7c0b20, at 0x7c0c18. When the resource is mid-async-read (`m_dwFlags & 0x10`) but
  `m_pUnk50` is NULL, the function returns `m_pFileData` directly — every other exit that hands
  back data goes through `m_nDemandCount++` first. A caller that Demand()s on that window gets the
  pointer without registering a hold, so the matching Release() decrements a count it never
  incremented.

## `CExoResMan::GetNewResRef` calloc's the trailing-number buffer one byte too small

- **Where:** @0x7c37a4, at 0x7c38b8..0x7c38e0. The base-stem copy is sized correctly
  (`calloc(nBaseLength + 1, 1)` then `strncpy(szBase, szResRef, nBaseLength)`), but its number-half
  twin allocates exactly `nLength - nBaseLength` bytes (`subs w8,w23,w25` feeds `calloc` unchanged,
  no `+ 1`) and then `strncpy`s that same count into it. strncpy only NUL-terminates when it runs
  out of source, so a full-length digit run fills the whole allocation and leaves no terminator —
  and the very next statement is `atoi(szNumber)`, which reads past the end. The calloc-zeroed
  slack the code is implicitly relying on is exactly the byte it forgot to ask for.
- **Then it overwrites it anyway:** the same buffer is immediately `sprintf(szNumber, "%d", …)`-ed
  and re-used as the loop's digit-count probe, so a ResRef whose stem is shorter than its number
  (e.g. `a12345678`) overflows the heap block on the FIRST sprintf too.

## `CExoMemMan::CheckHeaps` never advances the list pointer

- **Where:** @0x7b0e6c. The body is `pHeap = GetHeaps(); while (pHeap) pHeap->CheckHeap();` — there
  is no `pHeap = pHeap->m_pNext` anywhere in the 60 bytes, and `x19` is untouched across the
  virtual call, so the loop back-edge at 0x7b0e98 re-enters with the same node forever. It hangs on
  the first registered heap. It survived because nothing in the shipping build ever registers one
  (`CExoMemHeap::m_lHeapsStart` is permanently NULL — the class has no vtable and no subclass in
  the image), so CheckHeaps' loop is never entered at all.
- Both siblings do it right: `CExoMemMan::Heap_Walk` @0x7b0f00 and `PrintHeapWalk` @0x7b1554 each
  end their body with `ldr x20,[x20,#8]`.

## `CExoMemMan::Clear` zeroes one byte past the array it is clearing

- **Where:** @0x7b0d38, at 0x7b0c90/0x7b0d7c: `memset(this + 8, 0, 0x401)`. The field at +0x008 is
  0x400 bytes — the next byte, +0x408, is `m_aTypeStack[0]`. Harmless in practice for the one
  reason that is easy to miss: the type stack is 1-BASED (the constructor sets
  `m_nTypeStackDepth = 1` and PushType stores at `[m_nTypeStackDepth]` before incrementing), so
  slot 0 is the one byte in the array that is never read.

## `CExoMemMan::Heap_Walk` stamps every report file with the wrong month, and leaks on a half-open pair

- **Where:** @0x7b0f00. `pTime->tm_mon` is passed to `"logs\HeapContents_%s_%d%02d%02d%02d%02d.txt"`
  raw (0x7b0fa8-0x7b0fcc computes `tm_year + 1900` but nothing for the month), so a January walk
  writes `..._2004000...` — `tm_mon` is 0-based.
- Both files are opened before either is tested (0x7b101c and 0x7b105c) and the guard is
  `if (pContents && pStats)`. When exactly one `fopen` succeeds the function skips straight to the
  next heap without closing the one it got.

## `CExoMemMan`'s constructor leaves two of its own flags uninitialised

- **Where:** @0x7b0c28. The single 8-byte `str d0` at 0x7b0cd8 initialises `m_bTrackFileLine`
  (+0x4e47e4) and `m_bMemTrackEnabled` (+0x4e47e8) from a `{1, 0}` constant pool pair, and
  `m_nCommentCount` (+0x4e47e0) and `m_bMemTrackStats` (+0x4e47ec) are never written at all —
  neither by the constructor nor by `Clear`. It only works because the sole instance is the
  function-local static inside `GetMemMan()`, so it starts life zeroed in .bss.

## `CExoInputInternal::RemoveEvent` never compares anything when it scans for the node to unlink

- **Where:** @0x7aa7cc, at 0x7aa8b8-0x7aa8fc. The scan calls
  `CExoLinkedListInternal::GetAtPos(pNode)` and then **discards the result**; the value the loop
  tests against `nEvent` is a local that is initialised to 0 and never reassigned. Both toolchains
  emit the giveaway verbatim — arm64 `bl GetAtPos ; cbnz w20, <found>` and the macOS oracle
  `callq GetAtPos ; testl %r13d,%r13d ; jne` — so this is the source, not a codegen artifact.
- Net effect: the match test degenerates to `nEvent != 0`. Removing any non-zero event id unlinks
  whatever node happens to be at the head of that class's device list, and removing event id 0
  walks the whole list and then returns −1 without unlinking anything.
- The sibling walkers get it right: `IsControlUsed` @0x7aa280 and `AddEvent` @0x7aa564 both do
  `m_aEvents.m_pArray[*(int*)GetAtPos(pNode)]` and compare the descriptor they get back.

## `CExoInputInternal::RemoveEvent` clears a whole bitmask word instead of one bit

- **Where:** @0x7aa7cc, at 0x7aa938: `str wzr, [x9, w8, sxtw #2]` — a plain zero store into
  `m_aEventClass[nClass].m_aEventBits.m_pArray[nEvent / 32]`. `AddEvent` @0x7aa7bc sets its bit
  with a read-modify-write `orr`, so the two are not symmetric.
- Net effect: removing one event silently deregisters the other 31 ids that share its 32-bit word
  from that class. Survivable in practice only because the shipping binding set is built once at
  startup (`CClientExoAppInternal::SetEventDescriptions` @0x46abd4) and torn down wholesale via
  `ClearAllInputEventDescriptions`; the one live caller of the pair,
  `CExoInputInternal::ScaleEvent` @0x7aa97c, re-adds the same id immediately afterwards, which
  restores only that id's bit.

## `CExoInputInternal::ScaledValue`'s joystick-slider range test is a vacuous `&&`

- **Where:** @0x7aadd4, at 0x7aaea0-0x7aaed0. The guard reads
  `nControl < JOYSTICK_SLIDER0 && nControl > JOYSTICK_SLIDER2` (113 and 115) — an `&&` where `||`
  was plainly meant. No value can be both below 113 and above 115, so the condition is always
  false and the "not a joystick control" early-out is dead.
- arm64 computes it branchlessly and gives the game away (`cset w8,hi` / `cset w9,lo` / `and` /
  `tbz` — an AND of two mutually exclusive predicates); the macOS oracle emits the same shape as
  `jbe <joystick>` / `jb <return>`, whose fall-through is also unconditional.
- Net effect: every control that is neither in the Return..Space key band nor a mouse axis is
  normalised against `GetMinUseable`/`GetMaxUseable` as though it were a joystick axis, including
  ordinary keyboard and mouse-button codes.

## `CExoInputInternal::WriteEventToFile`'s guard is an `||` where it wants an `&&`

- **Where:** @0x7ab2ec, at 0x7ab31c-0x7ab32c. The body runs when
  `nIndex < 0 || pFile != NULL || nIndex >= m_nUnk260`, i.e. a NULL `pFile` combined with an
  out-of-range index still walks `m_pEventQueue` and then calls `CExoFile::Write` on NULL.
- Only reachable from `DumpEventsToFile` @0x7ab268, which always passes a caller-supplied file and
  an in-range index, so it never fires.
- Related: `DumpEventsToFile` gates on `m_nEventQueueCount != 0` but bounds its loop with the
  *other* count at +0x260, and `WriteEventToFile` range-checks against that same +0x260 rather
  than the queue's own length.

## `CExoInputInternal::CreateNewEvent` reports success after storing a NULL descriptor

- **Where:** @0x7aa370, at 0x7aa4e4/0x7aa558/0x7aa55c. An event class outside 0..5 falls to the
  switch's default, which sets the descriptor pointer to NULL, stores that NULL into
  `m_aEvents.m_pArray[nEvent]` and still returns TRUE. Every other reject path (device > 5, slot
  already occupied) returns FALSE without touching the array.
- Net effect: the caller believes the event exists, `CExoInput::EventExists` @0x7b07fc correctly
  says it does not, and the next `AddEvent`/`PollInput` for that id dereferences NULL.

## `CExoInputInternal::BufferEvent(void*)` extracts the middle-button coordinates UNSIGNED

- **Where:** @0x7ad028, cases 0x207 (WM_MBUTTONDOWN) and 0x208 (WM_MBUTTONUP). Every other mouse
  message extracts the click position from `lParam` the Win32 way — `GET_X_LPARAM`/`GET_Y_LPARAM`,
  i.e. `(short)lParam` and `(int)lParam >> 16` (`sxth` + `asr` in arm64, `movswl` + `sarl` on
  macOS). Those two cases use `(unsigned short)lParam` and `(unsigned)lParam >> 16` instead
  (`and`/`lsr`, `movzwl`/`shrl`) — the classic `LOWORD`/`HIWORD` mistake.
- **Oracle-confirmed:** the macOS build makes the same split, so this is SOURCE, not codegen.
- Net effect: a middle-button click at a negative client coordinate — which happens on a
  multi-monitor desktop, or during a drag that leaves the window — is queued as ~65535 instead of
  a small negative number. Only the middle button is affected.

## `CSWParty::Reset` passes an UNINITIALIZED float to ProcessPoint (all three call sites)

`CSWParty::Reset(const Vector&, float, int)` @0x41f088 calls
`CSWPartyFollowDataArray::ProcessPoint(Vector const&, float, float)` three times and never writes
the third argument's register at any of them: arm64 leaves `s1` holding whatever the quaternion
block left there (`q.w` on the identity fast path, the un-scaled rotated `.y` on the slow path),
and the macOS build leaves `xmm1` equally untouched. A register the caller never writes is exactly
what clang emits for an `undef` argument, i.e. an uninitialized local in the source.
- **Oracle-confirmed:** the macOS build has the same hole, so this is SOURCE, not codegen.
- Net effect: ProcessPoint's third parameter is the TestDirectLine hit-fraction scalar elsewhere
  (see `CSWSCreature`'s SetPrecalculatedPosition path). Reset has no such probe to feed it, so
  every sample the party's follow path is seeded with on a reset carries a garbage scalar. The
  value is stack/register residue, so it is deterministic per build but meaningless.

## `CSWPartyFollowDataArray::ProcessPoint` throws away the distance it just measured

@0x41f3b4's snap-to-follow-point path (`|vFollowPos - vPos|² <= fDistance²`) commits the follow
point, stores `nFollowIndex` through the out-parameter — and then calls
`magnitude(vFollowPos - vPos)` whose result no register or stack slot ever receives.
- **Oracle-confirmed:** identical on macOS (`__Z9magnitudeRK6Vector` at 0x10025298a, result
  overwritten by the `-0x4c(%rbp)` reload two instructions later), so this is SOURCE.
- Net effect: `AdvancePointOnPath` reports its FULL distance budget as still unspent whenever the
  walk snaps onto the follow point, instead of subtracting the distance it just covered. Its one
  result-consuming caller (`WalkUpdateLocation_QuickWalk_FollowLeader_Following_Path` @0x54b2b4)
  therefore over-credits the remaining walk on exactly the frame the companion arrives.

## `CSWPartyFollowDataArray::SmoothFollowPath` line-tests both companions from the LEADER

@0x420190 resolves a server creature twice, back to back, to run the two per-companion
line-of-walk tests that decide whether each companion's follow-sample index can jump to the newest
sample. Both lookups pass `m_pParty->m_members[0].id` — the party LEADER — even though the two
results are stored into `m_members[1].m_bFollowPointValid` and `m_members[2].m_bFollowPointValid`.
- **Oracle-confirmed:** the macOS build makes the same two calls with the same id
  (`-0xb00(%rbp)`, loaded from `0x28(%rax)` off `m_pParty`), so this is SOURCE.
- Net effect: neither companion's own position is ever tested; both indices are re-seated (or not)
  purely on whether the LEADER can see the newest sample. The two calls are otherwise a verbatim
  copy-paste pair, which is what the bug looks like at the source level.
- Side note, same function: the function-local static `OFF` is bumped by 6 and wrapped at 40 on
  every call and is never read anywhere in either image — a leftover debug counter.

## `CSWParty::Update` re-measures a loop-invariant distance every iteration

Not a correctness bug, but worth recording next to the two above: `AdvancePointOnPath` @0x420c8c
recomputes `|vFollowPos - vPos|²` against the ORIGINAL distance budget inside its walk loop, from
the two `const Vector&` parameters — neither of which the loop writes. It is invariant, so it fires
on the first iteration or never; the compiler cannot hoist it only because `vOut` may alias.

## `VertexPrimitiveFlat::SetupArrays` calls `AsVertexPrimitiveDangly()` and discards the result

@0x7849d4 opens with two back-to-back downcast probes on `this`: slot 4
(`AsVertexPrimitiveAnim`, kept in x20 and used for the +0x28/+0x2c pool pair) and slot 5
(`AsVertexPrimitiveDangly` @0x784a08) — whose result x0 is overwritten two instructions later by
the `AsPartTriMesh` receiver load and is never read on any path.
- The call cannot be deleted because it is an out-of-line virtual, so what survives in the image
  is a dead local whose initialiser has unknown side effects. At the source level this is the
  familiar copy-paste of the line above it: the function needs the Anim probe and got the Dangly
  one for free.
- Net effect: one wasted virtual dispatch per SetupArrays. ⭐ v1996 CLOSES the "and the DANGLY
  path is visibly unfinished" half of this entry: `VertexPrimitiveDangly`'s per-vertex `Vector`
  array (+0x28, `m_paVertices`) is **not** a draw-time binding and never was — the macOS
  `PartDanglyMesh::Animate` @0x1000eaab4 uses it as the pendant solver's per-vertex VELOCITY state
  (read at the top of its per-vertex loop, written back at the bottom). There was nothing here for
  `SetupArrays` to bind, so the only defect in this function is the copy-pasted dead probe. The
  array LOOKS orphaned in the arm64 image because the port cut the solver (`docs/PORT_CUTS.md`),
  leaving it allocated, zeroed and unread.

## `GetSkinVP`'s hologram program falls back to the NON-holo blob

@0x6f8610 builds both skin vertex-program singletons on every call and picks between them on its
`bHolo` argument. The holo singleton is
`new VertexProgram(usearbvertexprograms ? cHoloSkinVPARB : cSkinVP, cSkinVPDeclaration)` — the
`csel x1, x22, x9, eq` at 0x6f86a8 selects x22, which is `cSkinVP`, the *plain* skin program, for
the non-ARB arm. There is no `cHoloSkinVP` symbol anywhere in the image (only `cHoloSkinVPARB` and
`cHoloVPARB`), so the fallback is not a mis-typed name for a real blob.
- Net effect: on any device without ARB vertex programs, hologram-rendered skinned meshes get the
  ordinary skin program and lose the hologram effect entirely, silently. Both `skinvp` and
  `holoskinvp` are also constructed unconditionally even when only one of them can be returned.

## Three bump-highlight console commands are compiled in but never registered

`usebumpdiffuseonly` @0x6f88f4, `usebumpspecularonly` @0x6f891c and `usebumpspecularanddiffuse`
@0x6f8944 are complete console handlers — each sets the `usebumpdiffuse` / `usebumpspecular` pair
the bump passes read and returns its acknowledgement string. Nothing in the image references any
of them: there is no relocation to their addresses and no `"usebumpdiffuseonly"` command-name
string anywhere in the binary.
- Net effect: the commands are unreachable; only their default-visibility definitions (and the
  three ~60-character strings) survive in the shipped library.

## `InitGamepadMenu` calls `GetServerCreature()` and throws the result away

`CSWGuiMainInterface::CSWGamepadMenuIos::InitGamepadMenu` @0x434940, in the loop that lights one
portrait per party companion, does `bl CSWCCreature::GetServerCreature()` at 0x434ae4 between the
`pMember != NULL` check and the `i < pParty->m_nCharacterCount` test, and nothing reads x0
afterwards — the next instruction is `ldrsw x8,[x22]`, reloading the party count.
- Net effect: one wasted virtual-free call per party slot per menu open, and (more usefully as
  evidence) a dead local in the original source that clang could not eliminate because
  `GetServerCreature` is not `readnone`. It reads like a leftover from a check that was removed:
  the natural guard would have been `if (pMember->GetServerCreature() != NULL && ...)`.

## `CSWGuiMainInterface::CSWGuiActionMenuIos::HandleInputEvent` tail-calls with an unset argument

At 0x427e60 the 0x28 ("back") arm loads `m_pInterface` into x0 and branches straight to
`CSWGuiMainInterface::HandleButtonActionX(CSWGuiControl*)` without setting x1 at all — the callee
receives whatever nType left there. This is the same DeadArgumentElimination shape
`CSWGuiHelpPanel::HandleInputEvent` has on `HideHelp`, and it is only safe because
HandleButtonActionX never reads its parameter.
- Net effect: harmless as shipped, but the declared-and-unread parameter is load-bearing for anyone
  changing HandleButtonActionX's signature — giving it a live parameter would silently make this
  call site pass garbage.

## `CLYT` never initializes `m_bDemanded`, so `~CLYT` releases a resource the object may never have demanded

`CLYT::CLYT` @0x7c8770 zero-inits the four record counts (+0x34..+0x40) and all eleven array
pointers (+0x48..+0x98), but writes NOTHING to +0x28, +0x2c or +0x30. +0x30 is `m_bDemanded`, the
flag both `CLYT::UnloadLayout` @0x7c8954 and `~CLYT` @0x7c87f4 test before calling
`m_pRes->Release()` — and the only writers are the two `LoadLayout` overloads, which set it to 1.
- Net effect: a `CLYT` that is constructed and destroyed without a successful `LoadLayout` (the
  `.lyt` is missing, or `CRes::Demand()` returns NULL) reads an uninitialized flag; if the stack /
  heap slop happens to be nonzero it calls `Release()` on the base `CResHelper`'s `m_pRes`, which
  at that point is whatever `SetResRef` left there — an unpaired release, or a NULL dereference
  when `LoadLayout` was never called at all and `m_pRes` is 0.
- **Confirmation:** the macOS build's `CLYT::CLYT` @0x1002d9d0e does exactly the same thing —
  `movq $0,0x3c(%rbx)` / `movq $0,0x34(%rbx)` and then the 0x48..0x98 run, with +0x28..+0x33 never
  touched. Two compilers on two targets do not independently omit the same three stores, so the
  missing initializers are in the SOURCE. Transcribed verbatim (v969), so the bug is visible.
- +0x28/+0x2c are dead in every CLYT method; they are modelled as `m_Unk0x28`/`m_Unk0x2c` purely
  to hold the offsets.

## `CSoundSetList::CSoundSetList` loads soundsettype.2da for nothing, and leaks a C2DA on both failure paths

`CSoundSetList::CSoundSetList` @0x3c17bc builds its three parallel arrays out of **soundset.2da**
only. It nevertheless `new`s a second `C2DA(CResRef("SoundSetType"), FALSE)` and `Load2DArray()`s
it, then never reads a single entry — the whole row loop addresses `p2daSoundSet` (x20); the
`SoundSetType` handle (x21) is touched again only by the `delete` at 0x3c19bc.
- Net effect: one extra 2DA file read + parse per client startup, for nothing. It reads like a
  column lookup that was moved into soundset.2da and whose load was never removed.
- Two leaks in the same function:
  * if `soundset.2da` fails to load, `cbz w0` at 0x3c1830 jumps straight to the epilogue —
    `p2daSoundSet` is never deleted;
  * if `soundsettype.2da` fails to load, `cbz w0` at 0x3c1868 goes to 0x3c19c8, which deletes
    **only** `p2daSoundSet` — `p2daSoundSetType` leaks.
  Each leak is a whole `C2DA` (0x90 bytes plus its parsed row/column tables), and the object is
  unreachable afterwards because the ctor keeps no pointer to it.
- Transcribed verbatim (v969) as nested `if (Load2DArray())` blocks with a bare `else { delete
  p2daSoundSet; }`, which is the shape that reproduces the shared `blr` teardown tail byte-exactly.

## `PrintStringScreenPos_UseTextureFont` indexes the font list with -1 until `SetConsoleFont` runs

`PrintStringScreenPos_UseTextureFont` @0x6c5b7c reads the current console font as
`CAurFont::s_LoadedFontList.m_pData[consoleFont]` **twice**, with no bound or validity check.
`consoleFont` is a file-static in `.data` whose initialiser is **-1** (0xc64744 holds 0xffffffff in
the shipped image), and the only writer is `SetConsoleFont` @0x6c591c.
- Net effect: called before any `SetConsoleFont`, the `ldrsw x8,[x20,#0x744]` / `ldr x8,[x9,x8,lsl
  #3]` pair loads the 8 bytes **immediately before** the font array and dispatches
  `CAurTexture::Bind()` (vtable slot 47) through it. It is an out-of-bounds read followed by a
  wild virtual call, not merely a wrong glyph.
- `AurFreeFont` @0x6c5a5c makes it reachable a second way: it deletes the console `StringGob` and
  nulls the pointer but leaves `consoleFont` pointing at whatever index the freed font had.
- Two GL state changes (`glEnable(enableTranslationTable[0])` and `glBlendFunc`) also happen
  BEFORE the `pszText == 0 || *pszText == 0` early-out, so an empty string still leaves blending
  enabled — the matching `glDisable` is only on the drawing path.
- Transcribed verbatim (v970); the function is byte-matched apart from one scheduling swap.

## `PostedString::Post` saves and restores `g_cSavedCol` around nothing

`PostedString::Post` @0x6c60b8 copies the 16-byte `g_cSavedCol` into a stack temporary and writes
it straight back with no statement in between. The macOS oracle @0x1000a8893 does the full round
trip (`g_cSavedCol` → `-0x18(%rbp)`/`-0x10(%rbp)` → `g_cSavedCol`); the arm64 build keeps only the
outbound half. Two compilers on two targets emitting the same dead round trip puts it in the
SOURCE — presumably a save/restore whose body was `#ifdef`'d out of the shipped build.
- Harmless, but it is why the function does not byte-match under this TU's `-fbuiltin`: with
  builtins on, DSE removes the copy and 16 bytes with it.

## `CAurFont::CAurFont(int, char)` remaps the glyphs of the font it clones

`CAurFont::CAurFont(int nFont, char chChar)` @0x6d4120 builds the "every character draws as
`chChar`" font (password fields, bars) by copying `s_LoadedFontList[nFont]`'s `CAurFontInfo*`
POINTER into its own `m_pFontInfo` and then writing all 96 printable glyph entries of that table:

```
    m_pFontInfo = pFont->m_pFontInfo;               // ldr x8,[x23,#0x18] / stur x8,[x20,#-8]
    for (int i = 32; i < 128; i++)
        m_pFontInfo->m_paMin[i] = pFont->m_pFontInfo->m_paMin[chChar];   // same object
```

Both sides of every assignment resolve to the SAME `CAurFontInfo`, and the arrays it points at are
the ones `CAurTexture::GetFontInfo()` hands out — i.e. they belong to the shared texture, not to
either font. Constructing one of these permanently turns the source font (and every other CAurFont
sharing that texture) into the same solid block; the two objects are otherwise indistinguishable,
so nothing can undo it.
- The compiler's two separate reloads of `+0x18` (`ldr x8,[x23,#0x18]` for the source and
  `ldr x10,[x19,#0x18]` for `this`) confirm the source really does spell both sides, rather than
  the destination being a private copy the ctor forgot to allocate.
- Only the `u`/`v` pair of each 12-byte `CAurGlyphCoord` is copied (one `ldr x` / `str x`), which
  is consistent with it being deliberate glyph remapping — just aimed at the wrong object.
- Transcribed verbatim (v971).

## `Scene::RenderGobBefore` writes one slot BEFORE the object array when the anchor is missing

`Scene::RenderGobBefore(CAurObject* pObject, CAurObject* pAnchor)` @0x77418c re-orders the draw
list so `pObject` renders immediately before `pAnchor`. It finds the anchor's index, re-appends the
object, rotates the tail down and finally stores the object at the anchor's old slot:

```
    int nIndex = -1;
    for (int i = 0; i < m_objects.m_count; i++)
        if (m_objects.m_pData[i] == pAnchor) { nIndex = i; break; }
    m_objects.Remove(pObject);
    m_objects.Add(pObject);
    for (int i = m_objects.m_count - 1; i > nIndex; i--)
        m_objects.m_pData[i] = m_objects.m_pData[i - 1];
    m_objects.m_pData[nIndex] = pObject;      // nIndex may still be -1
```

The `-1` sentinel is never tested. If the anchor is not in the scene's object list the rotate runs
all the way down to index 0 and the final store lands on `m_objects.m_pData[-1]` — eight bytes
before the heap buffer `List<Gob*>::allocate` handed out, i.e. inside the allocator's own bookkeeping
for that block. The object itself is also left duplicated at index 0.
- Ground truth, not inference: `mov w22,#-1` sits on the search loop's fall-through edge and w22
  flows into the same `sxtw x9,w22` / `str x19,[x8,x9,lsl #3]` pair the found path uses, with no
  compare or branch between them.
- ⭐ The sibling `RenderGobAfter` @0x773fbc CANNOT hit this: its sentinel is `0` (`mov w22,wzr`),
  so a missing anchor simply moves the object to the front. The asymmetry is what makes the `-1` a
  slip rather than a convention — the two functions are otherwise line-for-line the same shape.
- Transcribed verbatim (v972).

## `CSWRules::LoadFeatInfo` reads the "MinSpellLvl" column and THROWS THE VALUE AWAY
- `feat.2da`'s `MinSpellLvl` column is looked up, `GetINTEntry`'s success is discarded, and the
  field it should have gone to (`CSWFeat::m_nMinSpellLvl`, +0x2f) is then zeroed unconditionally:
  `bl C2DA::GetINTEntry` … `bl CExoString::~CExoString` … `strb wzr,[x8,#0x2f]` with **no `cbz`
  on the result**, unlike every other guarded column in the same loop. So the column is dead and
  the field is always 0 for every row loaded from the 2DA.
- Ground truth, not inference: the other three unconditionally-written columns (`GainMultiple`,
  `AllClassesCanUse`, `HostileFeat`) all CONSUME the value they read; this one is the only read in
  the loop whose result reaches no store at all.
- ⚠ Do not "fix" it: transcribe it read-then-zero, because that is what the bytes say.

## `CSWRules::LoadClassInfo` keeps a `CExoString` alive across its challenge-rating fill loop and never touches it
- The per-level challenge-rating run is filled inside a scope that CONSTRUCTS a default `CExoString`
  (`bl CExoString::CExoString()` at 0x533af0) and DESTROYS it immediately after the loop
  (`bl CExoString::~CExoString()` at 0x533b14) with nothing in between but the fill — a leftover
  from a column reader that was removed. It is emitted (a non-trivial ctor/dtor is not eliminable),
  so the row needs the unused local to match; it is tagged in-source as a measured artifact, not a
  style choice.

## The minimap arrow shows the CAMERA's facing, not the player's

- **Where:** `CSWGuiMainInterface::DrawMap`. The map image `m_miniMapImage` is only ever
  TRANSLATED (its extent is recomputed from `GetPartyMemberMapLocation`), never rotated — so the
  arrow label is the only thing on the minimap carrying an orientation. And its rotation comes
  from the camera:

  ```cpp
  CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
  Quaternion q = pCamera->GetOrientation();
  m_UnkLabel0x7ac8.m_border.m_params.m_Unk0x0c =
      pArea->GetMapRotateCCWFromWorldOrientation(Vector(/* q's forward axis */));
  ```

  Nothing in the function reads the player creature's facing.
- **Impact:** rotate the camera while standing still and the "you are here" arrow spins with it.
  It normally goes unnoticed because the follow camera sits behind the character, so the two
  agree while moving; it is only wrong when they are decoupled.
- **Confirmation:** the macOS build is identical — `tools/oracle.py
  _ZN19CSWGuiMainInterface7DrawMapEf --calls` lists `CClientExoApp::GetModuleCamera()` ->
  `CSWSAreaMap::GetMapRotateCCWFromWorldOrientation(Vector)` in the same position and names no
  creature accessor at all, and the shipped KOTOR 1 Android APK shows it too (2026-10-13 play test).
  The Switch build, however, turns the arrow with the player character's facing, so the console
  lineage fixed it.
- **Fix (`K2_UI_XBOX` or `K2_FIX_ENGINE_BUGS`, both games):** the arrow takes the party leader's
  `CSWCObject::m_orientation` — the creature `GetPartyMemberMapLocation(0, …)` already places the blip
  for — and falls back to the camera while the party has no player character (module load, save
  restore). Reported from a hybrid play test (docs/KOTOR2_ui_variants.md §7r (d), §7s) and again on K1.

## `WinMessageHandler` hands `DefWindowProcA` the SWAPPED mouse message on the "Reverse Mouse Buttons" path

- **Where:** `WinMessageHandler` @0x6be168. Every mouse arm picks its outgoing event message — its own
  when `CClientOptions::m_bReverseMouseButtons` is clear, the OPPOSITE button's when it is set
  (0x201↔0x204, 0x202↔0x205, 0x203↔0x206) — and then passes **that same variable** to `DefWindowProcA`
  as the message. The swapped value reaches it on the shared send tail: `mov w22,#0x205; b #0x6be53c`
  (the 0x202 arm's swap edge) falls through the `BufferEvent` sequence into `b #0x6bebc4`, whose
  `mov w1, w22` is the `DefWindowProcA` argument.
- **Impact:** an incoming `WM_LBUTTONUP` with the option set returns
  `DefWindowProcA(hWnd, WM_RBUTTONUP, …)` — the returned `LRESULT` belongs to a message the window
  never received. Benign for the button/wheel family (their defaults are 0), but the same tail is
  shared by `WM_MOUSEMOVE`/`WM_MBUTTON*`, so any future arm that returns something meaningful from
  DefWindowProc inherits the wrong message id.
- **Confirmation:** the swap targets are read straight off the orig's jump table and its per-arm block
  sequence; the same value is what lands in the message record's `message` field, so one variable
  feeds both the event and the default handler. Transcribed verbatim (v1661) with an arm-local
  `nMessage` used in both places so the leak stays visible.

## Every mouse message `WinMessageHandler` queues carries an UNINITIALISED `hwnd`

- **Where:** `WinMessageHandler` @0x6be168. All nine mouse arms build their `MSG` at `sp+8` and write
  only `message` (+0x08), `wParam` (+0x10) and `lParam` (+0x18); the `hwnd` field at +0x00 is never
  stored, before or after the call.
- **Impact:** `CExoInput::BufferEvent(void* pMessage)` receives a record whose first 8 bytes are
  whatever the stack frame held. Harmless today only because that function reads nothing but
  `message` and `lParam` (src/libsource/exoinput/CExoInputInternal/CExoInputInternal.cpp) — a
  consumer that ever looks at `hwnd` would read stale stack.
- **Confirmation:** the frame's canary sits at `sp+0x38`, so a full 0x30-byte `MSG` at `sp+8` abuts it
  exactly and the three written fields land inside it — i.e. the object IS the whole record, not a
  narrower struct. An out-of-line extern callee (`CExoInput::BufferEvent` @0x2f91e0) forbids DSE, so
  the missing store is genuinely absent from the source rather than optimised away.

## `delete[]` in ten texture controllers' destructors, on pointers two of the ctors never initialise

- **Where:** the `TextureController` subclass family (`src/game/TextureController/TextureController.h`
  and its ten sibling headers). `LifeTextureController` and `PerlinTextureController` are constructed
  by `CAurTextureBasic::ParseField` (@0x700c34) as `operator new(0x58)` / `operator new(0x68)` + the
  base ctor + the vptr store and **no member store at all** — their own member store is absent from
  the factory in every case, because their ctors have empty bodies. Their (inline, weak) destructors
  then `delete[]` +0x50 (Life) and +0x50/+0x58/+0x60 (Perlin, where +0x60 is only an int) — i.e. the
  destructor reads and frees members that were never written.
- **Impact:** any Life or Perlin texture destroyed before its `Init()` has run (a failed construction,
  a `proceduretype` line that names the class and then overwrites the controller, or simply a load
  that never reaches Init) frees whatever the allocation happened to contain. In the happy path
  `Init()` allocates both pointers first, which is why this survives in shipped builds.
- **Confirmation:** three independent reads agree. (1) The factory's `life` arm is `operator new(0x58)`
  + `bl TextureController::TextureController` + `str x8,[x21]` and nothing else; the `perlin` arm is
  the same with 0x68. (2) There is no `_ZN{21Life,23Perlin}TextureControllerC1EP11CAurTexture`
  anywhere in the .so, so no out-of-line ctor body exists to hold the missing stores. (3) By contrast
  the sibling `RandomTextureController`/`RingDistortTextureController` arms DO store zero to their
  pointers (`str xzr,[x21,#0x50]` …), which is what an initialised member looks like in the same
  function. Transcribed verbatim (v1662): the ctors are empty and the destructors still `delete[]`.

## …and a third one: `HeatDistortTextureController` is a class the shipping engine never instantiates

- **Where:** `HeatDistortTextureController` (`_ZTV28HeatDistortTextureController` @0xc24dc0). Every
  other subclass has an arm in the `CAurTextureBasic::ParseField` name table and a matching
  `operator new`; this one has neither, and the ONLY references to its vtable anywhere in the .so are
  the GOT slot 0xc60850 (read by its own two destructors) and the entry point's RTTI sweep.
- **Impact:** none at run time (nothing constructs it) — but it matters for the model: its `sizeof` is
  UNOBSERVABLE from this binary, so its member map cannot be pinned the way the other nine were (the
  `new` immediate is the usual oracle). It is modelled as one `unsigned char*` at +0x50 → 0x58 B by
  analogy with `LifeTextureController`, and `Init()`/`Control()` are out-of-line EMPTY definitions —
  they are the class's key function, which is why the vtable (and thus its two weak destructors)
  exists at all.
- **Confirmation:** `get_xrefs_to` on the vtable returns only the GOT slot; the ParseField name table
  at 0x9d20a2.. reads `dirty/dirty2/dirty3/water/life/perlin/arturo/wave/cycle/random/ringtexdistort`
  with no heat-distort spelling; and the two 4-byte `ret`s at 0x6c513c/0x6c5140 are `T` (global)
  rather than `W`, i.e. out-of-line definitions.

## `CResGFF::WriteFieldCExoString` — an EMPTY string's length word is never written

- **Where:** `CResGFF::WriteFieldCExoString` @0x7cf968. It reserves `nLen + 4` bytes through
  `AddDataField` and stores the length word only under `cbz w22` (0x7cfa28) — i.e. only when the
  string is non-empty. `AddDataField` (@0x7ccdbc) and `InitializeForWriting` grow the field-data
  buffer with plain `operator new[]` and never zero it, so for an empty string the 4-byte length is
  whatever the heap held.
- **Impact:** every empty `CExoString` field in a written GFF reads back with a garbage length. On
  the macOS host this corrupts `savenfo.res` (`SAVEGAMENAME` when no description is typed, and
  `LIVE1`..`LIVE6`, which are always empty) — the readers then pull hundreds of bytes of neighbouring
  field data into the save name. The Android build presumably survives on allocator luck (fresh
  zero pages).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** an `else *pData = 0;` arm after the guarded store.

## `CSWSArea::UpdatePositionInObjectsArray` — an object missing from the X index swaps index -1 into slot 0

- **Where:** `CSWSArea::UpdatePositionInObjectsArray` @0x688a8c. The search loop leaves
  `w24 = -1` when the object is not in `m_paObjectsByX`, and nothing tests it: `-1 + 1 != count`
  takes the bubble-right arm, which (when slot 0 sits left of the object) swaps
  `m_paObjectsByX[-1]` — the heap word in front of the array — with `[0]`.
- **Impact:** a foreign 8-byte value enters the area's object index. On Android it is usually junk
  that names no object and is skipped. On the macOS host (zeroed heap) it is id **0 = the module**,
  so `~CSWSModule` → `CSWSArea::ClearObjects` deletes the module again → unbounded recursion: the
  crash on loading a game from the in-game menu (v1997).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** return early when `nIndex == -1`.

## `CSWSDoor::LoadDoor` — `OnDamaged` defaults to the OnClosed script

- **Where:** `CSWSDoor::LoadDoor` @0x538994. Every script slot is read as
  `m_scriptSlots[i] = ReadFieldCResRef(…, "<label>", …, CResRef(m_scriptSlots[i]))`, except
  `OnDamaged`: its default is `CResRef(m_scriptSlots[1])` (the OnClosed value read one statement
  earlier — the orig keeps `x22 = this+0x340` live across both calls and stores to `+0x350`).
- **Impact:** a door GFF with no `OnDamaged` field gets the door's OnClosed script as its
  OnDamaged script instead of keeping the previous value (empty on a fresh door). Rare: blueprints
  normally carry every field.
- **Almost certainly** a copy-paste of the previous line's default. Reproduced (v2089, found by
  `tools/argpath.py`).

## `CERFFile::RemoveResource` — frees the matched key twice and reads it after the first free

- **Where:** `CERFFile::RemoveResource` @0x7b865c. The match is found on a `m_lstKeys` node, and
  the resource is then fetched as `m_lstRes->GetAtPos(thatNode)`. `GetAtPos` only reads the node it
  is handed, so this returns the node's own data, which is the `CERFKey` (0x7b86fc-0x7b8704). The
  orig deletes the key, reads `pRes->m_pData` (+8, freed memory), deletes that, and deletes the key
  a second time. The matching `CERFRes` is never freed or unlinked from `m_lstRes`.
- **Impact:** heap corruption on any call. No caller in `src/` calls it, so nothing reaches it
  today; ASan would flag it at once if a host-build path ever does.
- **Status:** transcribed verbatim (found v2101, list C of the asymmetry-note read); no fix yet.

## `ExecuteCommandGetInPersistentObject` — unchecked `AsSWSCreature()` on the area-of-effect scan

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandGetInPersistentObject` @0x575778, the
  area-of-effect arm. After a candidate passes `InAreaOfEffect` and `GetDead() == 0`, the orig calls
  `AsSWSCreature()` again (0x575d18) and reads `m_bInParty` at `[x0,#0x17b4]` with no null test.
- **Impact:** a null dereference for any NON-creature candidate (placeable, door, item, …) whose
  object-type filter passes, i.e. when the script asks for `OBJECT_TYPE_ALL` (0x7fff) or a
  non-creature type. On Android the read at 0x17b4 faults. Scripts in practice ask for creatures,
  which is presumably why it never showed.
- **Status:** transcribed verbatim (found v2101, list C of the asymmetry-note read); no fix yet.

## v2102 "the orig's own" note read — original-game bugs reported (orig addresses; details in `docs/prep/orignote_v2102_{a,c,d}.md`)

Reported by the read-only note read and transcribed as the orig has them. Each was read on the orig by
one agent; none is fixed (no `K2_FIX_ENGINE_BUGS` arm yet).
- **`CResSET::CreateSectionTable`** (0x3efb0c-0x3efb60): on an LF-only file a line goes through
  `__strncpy_chk(buf, p, m_nSETSize - idx, 256)` (fortify abort past 256 B) and then
  `buf[m_nSETSize - idx] = 0` (out-of-bounds stack write). The trailing-whitespace trims
  (0x3efc28, 0x3efccc) never store.
- **`CSWSEffectListHandler::OnRemoveSetState`** (0x663a50-0x663a64): the stale-subType-9 walk does not
  advance after `RemoveEffectById`; if the effect is not actually removed it spins forever.
- **`Parse(char*, AABB*&)`** (0x75cb18-0x75cb64): the ±0.01 inflation is applied after the AABB is built.
- **`TextureController::Height2Channel`** (0x6c1cd4): the high-nibble store overwrites the low nibble.
- **`CFactionManager::LoadFactionsFromSaveGame`** (0x69485c): a failed read resets bSuccess to 1;
  always returns 1.
- **`CSWSCreature::RunUnequip`** (0x623848): the 2/3 arm messages a possibly-NULL partner.
- **`CServerExoAppInternal::TempDeathUpdate`** (0x603d08): `GetArea` on a NULL `pLastCreature` when the
  last party slot has no server creature. Fixed under `K2_FIX_ENGINE_BUGS` (2026-10-22): the last NON-NULL
  party creature.
- **`CSWSPlaceable::EventHandler`** (0x6a66ec): the trap trigger dereferences `pCreature` unchecked;
  **`CSWSDoor::EventHandler`** (0x537c9c): same for `pTripper`.
- **`ExecuteCommandPlayAnimation`** (0x564234): `pObject` uninitialised when `m_nScriptRunArg != 1`.
- **`HandleServerToPlayerDoorUpdate_Add`** (0x3a34ac / 0x3a37e0): +0x17d stored only for door type 0, read always.
- **`HandleServerToPlayerCCMessage`** case 3 (0x3b1ea8): the 0x289 fallback name is dropped for an
  unassigned `sSpeaker`; case 16 (0x3ade8c vs 0x3adeac): the gate and the shown value read different slots.
- **`CSWGuiMessageBox::FixMessageLabel`** (0x3e4310-0x3e431c): Cancel gets OK's whole extent.
- **`CSWGuiScrollBar::HitCheckScrollbar`**: the horizontal left-arrow zone spans nX..nX+nW+dirW.
- **`CExoKeyTable::AddKeyTableContents`** (0x7b43f8) and **`AddResourceImageContents`** (0x7b407c):
  leaks on failure paths. **`CExoKeyTable::AddKey`** (0x7b38f4 → 0x7b3940): loops forever on a full
  table; `crc % m_nTableSize` has no zero guard (0x7b38c4).
- **`ImageWriteTGA`** (0x780ecc / 0x780f48): `new[]` freed with scalar `delete`.
- **`LightningEmitter::UpdateBranches`** (0x75177c-0x751794): branch axis `normalize(p[1] - p[1])`.
- **`CSWGuiListBox::HandleMouseCapturedMovement`** (0x80a694-0x80a6a0, Android port block): the touch
  dead zone needs BOTH axes past the threshold, so a purely vertical drag never scrolls.
- **`CSWGuiUpgrade::FinishUpgrading`** (0x4b8274): the re-select loop is `while (count != 0)`; an item
  missing from the rebuilt list never ends it and reads `GetControl(i)` past the end.
- **`converttobinary`** (0x70ca2c/0x70ca84, dev console): `__strlen_chk(szFile - 5, 0)` reads before
  the buffer and aborts under bionic's bound of 0.
- **`CScriptCompiler::CompileFile`** (0x7ea808): on a parse failure it releases the include slot one
  past the loaded script and never releases the script.
- **`CScriptCompilerInternal::TestIdentifierToken`** (0x7e7e44): `"break"` compared with n = 4, so any
  5-letter identifier starting "brea" lexes as `break`.
- **`SortObjectsForGameObjectUpdate`** (0x5e8890): a null `pObject` reaches `AsSWSCreature()`.
- **`CScriptCompilerInternal::InVisitGenerateCode`** `||` arm (0x7f03c4, verified v2103): the binary
  path stores opcode 0x1f (JZ) where the symbolic listing prints "JMP _ILO2_". The `LOGORII` arm
  of `PostVisitGenerateCode` (0x7f1044, 0x7f4130) back-patches only the offset bytes +2..+5, never
  the opcode. So with a true left operand the duplicated copy is non-zero, the JZ falls through, and
  the right operand is evaluated anyway: compiled `||` never short-circuits. The result is still
  correct (`LOGORII` of left and right). Only side effects in the right operand differ, and the
  symbolic and binary outputs disagree. `&&` (`JZ _ILA_`) is unaffected.

## v2103 note read — original-game bugs reported (orig addresses; details in `docs/prep/orignote_v2103_{a,b,c,d}.md`)

Reproduced by our source unless marked. None has a `K2_FIX_ENGINE_BUGS` arm yet.

- **`DoTimeTriggers`** (0x72a058): after a trigger's console command runs, the engine deletes
  `timetriggers[0]`, not the trigger it ran. A command that schedules an earlier trigger destroys the
  new one and leaves the fired one to fire again.
- **`CClientExoAppInternal::UpdateCombatMode`** (0x47102c): when a member's fallback `GetGameObject`
  misses, the door/placeable lock gate runs on the PREVIOUS member's `pplayer`.
- **`DecompressDXT1_1555`** (0x8f8100) — **reproduced (v2165); fixed under `K2_FIX_ENGINE_BUGS`.**
  The 1555 packing has a one-bit red field (`Color1555`'s `r:1` bitfield typo). Red keeps only its
  lowest bit, and bits 12..15 come from registers the loop never initialises (0x8f82cc..0x8f8354).
  The fixed build declares `r:5`. No direct callers.
- **`CSWCCreature::UpdateLookAt`** (0x498c34): the player scan compares `dist²` with a linear
  `fNearest`, where the area scan squares it (0x498ab4).
- **`CExoBaseInternal::GetDirectorySize`** (0x7a63e8) / **`CheckForCD`** (0x7a7dec): loop until
  `ERROR_NO_MORE_FILES`. Any other `FindNextFileA` failure spins forever re-adding the same entry.
- **`Scene::RenderGobAfter`** (0x774044): the inlined `List::Remove` shrinks the list even when the
  object is not in it. The anchor index is taken before the remove, so an object that preceded its
  anchor lands one slot late.
- **`CVirtualMachineStack::AddToTopOfStack`** (0x7f5db8): a float push stores 8 bytes from a 4-byte
  slot; the cell's upper half is stack garbage.
- **`MdlNodeTriMesh::InternalParseField`** (0x766b4c): once faces exist, every later field line
  re-runs `m_faces.allocate` and the per-face copy.
- **`setbsphere`** (0x6eb640 / 0x6eb5cc): no null test on `FindModel`'s result (unknown model name
  crashes); with one argument the radius scan reads one byte past the NUL.
- **`Moving_To_Path`** (0x54b020, probable inverted test): on `TestDirectLine == -3` it re-slots
  silently for a NON-party blocker and signals a party blocker with event 0x1f. Its siblings
  `Following_Path` / `Moving_To_Offset` do the opposite.
- **`GLRender::InitAttenuationTexture`**: uploads RGBA data as `GL_RGB8`, so the built alpha is
  dropped (harmless).

## v2104 note read — original-game bugs reported (orig addresses; details in `docs/prep/orignote_v2104_{a,b,c,d}.md`)

Reproduced by our source unless marked. None has a `K2_FIX_ENGINE_BUGS` arm yet.

- **`LifeTextureController::Control`** (0x6c3b44 / 0x6c3b54): leaks one image-sized `new[]` + `memcpy`
  per generation. Our source transcribes it, but `-fbuiltin` deletes the dead allocation, so OUR
  build does not leak.
- **`CSWGuiSwoopUpgrade`**: the mode byte at +0x5b49 is never stored as anything but 0 (whole-`.text`
  scan; the only writer is OnPanelRemoved's `strb wzr` at 0x4cf578), so every `== 1`
  ("3D_MODEL_LS") arm in OnSlotSelected / SetDescription / Update3DView / HandleInputEvent is dead.
- **`CSWGuiUpgrade::GetNumUpgrades`**: returns the unmasked shifted word (`0x02030306 >> 8n`); only
  the low byte is the count. It has no callers.
- **`CExoInputInternal::RemoveEvent`** (0x7aa8bc / 0x7aa938): re-confirmed (already logged): the
  match test is `nEvent != 0` and the bitmask clear stores a whole zero word.

## v2105 note read — original-game bugs reported (orig addresses; details in `docs/prep/orignote_v2105_{a,b,c,d}.md` and `docs/prep/loadmeshtext_v2105.md`)

Reproduced by our source unless marked. None has a `K2_FIX_ENGINE_BUGS` arm yet.

- **`CSWRoomSurfaceMesh::LoadMeshText`** (0x64f6e8, the ASCII `.wok` loader; vanilla ships binary
  walkmeshes, so only modded/ASCII rooms reach it):
  - the verts-loop and faces-loop aborts free only the line buffer (the faces abort leaks the
    vertex/material arrays and any maps from an earlier `faces` block); the `aabb` abort leaks every
    heap node already built;
  - a second `faces` block overwrites the walkable/exterior maps and face arrays without freeing them;
  - `aabb` before `faces` dereferences the NULL walkable map; `leafTri` is never bounds-checked;
  - a short AABB line (sscanf < 7 fields) leaves `leafTri` uninitialised, and a missing
    `orientation` angle is uninitialised too;
  - a failed `GetINTEntry` leaves `walk` holding the previous face's value (uninitialised on face 0);
  - a second `aabb` block appends a second tree whose child/root indices are local to it, so
    `m_rootAABBIndex` points into the wrong tree.
- **`CSWSArea::PackAreaIntoMessage`**: the map-pin loop is unsigned, so a negative
  `NW_TOTAL_MAP_PINS` script variable runs it ~2^32 times (effectively hangs the area pack).
- **`CAurGUIStringInternal::Draw`** (0x72e408..0x72e448): with no precomputed line offsets, the width
  fallback measures len(text) × the width of the line's FIRST glyph (the glyph index is hoisted out of
  the loop), so centred/right-aligned text is mis-measured.
- **`~PartLight`** (0x6e0774, 0x6e07dc → 0x6e07b8): a gob in `gobs` with a null light container is
  dereferenced at +0x28. Latent if every registered gob owns a container. ⚠ Ours skips the gob.
- **`GetDamageBonusString`** (0x370b6c): the negative-modifier arm drops the accumulated string
  (harmless: it is still "" there).
- **`CExoSoundInternal::Initialize`** (0x7d1fc8..0x7d1fd8): a NULL source in `m_lPlaying` makes the
  reap loop spin forever (a degenerate self-branch). ⚠ Ours dereferences the NULL instead; it cannot
  happen in practice.

## v2106 note read — original-game bugs reported (orig addresses; details in `docs/prep/orignote_v2106_{a,b,c}.md`)

Reproduced by our source. None has a `K2_FIX_ENGINE_BUGS` arm yet.

- **`CSWSCreatureStats::ReadStatsFromGff`** (0x5c448c, PC branch): the two `GetFLOATEntry(0xc, …)`
  results are unchecked, so a missing ranges.2da row 12 stores an uninitialised stack float into the
  owner's `+0x15b8`/`+0x15bc`.
- **`IosLabel::SetString`** (0x33c728) / **`FilterString`** (0x33c258), Aspyr port code: in
  newline-filter mode a `'\n'` landing at output index 0x7fe writes two spaces, `nOut` reaches 0x800
  and the terminator goes to `buf[0x800]`, one byte past the buffer. In `SetString` that buffer is
  the stack array `szFiltered`, so a filtered label of ≥ 2046 chars trips the stack protector.
- **Retracted:** the v366 entry "`CSWCArea::UpdateDayNightCycle` — the dawn ambient-Z lerp runs
  backwards" was a misreading of the shared tail 0x4a8a10's register roles; the orig's dawn arm is
  night→day on every component. The reversal was ours (fixed v2106).

## `AurSetPalette` passes an uninitialised `bool` to `AurReleaseRaster` (v2107)

`AurSetPalette` @0x6fe068 hands `AurGetImageWrapper` the address of a stack `bool bProcessed` and
then passes that byte to `AurReleaseRaster` (`ldrb w7,[sp,#0x24]`, 0x6fe244). The wrapper writes
`*pbProcessed` only on the processed path (0x6fdd10), and `AurGetProcessedRaster` is a stub that
returns 0 in this build, so the byte is always whatever was on the stack. With a non-zero byte,
`CAuroraInterface::ReleaseRaster` takes its processed-texture arm and the palette raster is never
released. Reached only through the debug console's `setpalette` / `setpalettes`. Reproduced as is
(the transcription keeps the wrapper `noinline` so the byte is not folded to a constant).

## `ExecuteCommandResistSpell` reports an uninitialised roll (v2108)

`CSWVirtualMachineCommands::ExecuteCommandResistSpell` @0x587e18 sends the second CC message 0x11
(the "Force Resistance" combat-log line, GUI string 0xa423) with `SetInteger(0, nRoll)`, the d20 kept
in w22 (`and w22,w0,#0xffff`, 0x5881f0). On the paths where `DoSpellLevelAbsorption` or
`DoSpellImmunity` already returned 1, no roll is made and w22 still holds the low half of a creature
pointer, so the log line prints that as the roll. Reproduced as is (`int nRoll;` is left unset on
those paths).

## `FModAudioSystem::~FModAudioSystem` — each map drain advances the iterator out of the node it just freed

- **Where:** `FModAudioSystem::~FModAudioSystem` @0x3450ec, all three loops (sounds, streams,
  channels). The body is `delete it->second; map.erase(it); ++it;`: after `__tree_remove` and
  `operator delete(node)` the successor is read from the freed node (0x3451d4 `ldr x8,[x20,#8]`).
- **Impact:** a use-after-free at shutdown. Because the loop always erases the smallest element,
  the stale step can skip the parent node, which then leaks (and a 2-node tree exits early). An
  allocator that scribbles freed memory turns it into a crash.
- **Confirmation:** found by the v2164 note-less DIFF read; the faithful transcription is a byte
  MATCH (1024 B). **Fixed under `K2_FIX_ENGINE_BUGS`** (`erase(it++)`).

## `ChangeCharacter` leaves `m_bCharacterChangeInProgress` raised after a full party cycle

- **Where:** `CClientExoAppInternal::ChangeCharacter` @0x47ee88 sets the flag (0x47f004) on every leader
  change; only `HandleServerToPlayerCreatureUpdate_ControlObject` @0x3a0320 clears it, reached from the
  PlayerInfo update when it names a creature other than `m_playerCreatureId`.
- **Impact:** `ChangeCharacterToNextLivingPartyMember` with both companions incapacitated rotates the party
  back to the leader within one frame; no ControlObject arrives and the character screen's level-up
  (A, X, touch; `CSWGuiInGameCharacter::HandleInputEvent` / `ShowLevelUpGUI`) stays refused until the
  next real switch. Confirmed live with lldb on the Thor (flag 1, leader == player creature).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the flag is raised only when the new leader differs from
  `m_playerCreatureId`.

## `m_oidGoingToBeAttackedBy` outlives an attacker that gives up ("stuck in combat mode")

- **Where:** `CSWSCreature::AddAttackActions` @0x624644 writes the attacker's id into the leader's
  `+0x780`; `CSWSCombatRound::EndCombatRound` @0x596928 is the only clear, and only when that object is
  dead or gone. `CClientExoAppInternal::UpdateCombatMode` holds the local member in combat mode while the
  id names a live hostile.
- **Impact:** a hostile that announces an attack then idles (no target, empty queue) keeps the combat HUD
  up indefinitely after the combat music ends. Seen live on the Thor (a `g_merc`, 86 HP, idle).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `UpdateCombatMode` treats such an attacker as gone, clears the
  field and drops combat mode.

## A placed mine's "spotted" animation never reaches the client

- **Where:** `CSWSMessage::ComputeUpdateRequired` @0x5dd1d4 sets update bit 4 (animation) only for item /
  door / placeable / area-of-effect objects. `CSWSTrigger::AIUpdate` @0x5a9288 sets a mine's 0x27a0
  (spotted) / 0x279f (hidden) through `CSWSObject::SetAnimation`, which only marks a dirty bit nothing reads.
- **Impact:** a trigger's animation is sent once, in the all-bits update after its add. A mine created
  before the next AIUpdate tick reaches the client with animation 0 and keeps it: no bubble (the model's
  additive spheres), and the client's trap pick test (`GetCurrentAnimation() == 0x27a0`) skips it, until
  a reload re-adds it. Confirmed live on the Thor (server 0x27a0 x6, client 0x27a0 x1).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a trap trigger raises bit 4 when its animation differs from the
  last update's.

## A combat-mode change mid-conversation replaces the dialog camera (Korriban, Sion's chase)

- **Where:** `CClientExoAppInternal::SetCombatMode` @0x482258 swaps the camera on every change
  (`SetCombatCamera` / `RestoreCamera`) with no regard for an active dialog camera (behavior 0x106d).
  `CSWParty::SetLeader` @0x41d61c mirrors the new leader's combat bit into it, so a script's
  `SetPartyLeader` inside a conversation can flip it.
- **Impact:** every later `UpdateShotCamera` finds no dialog behavior and is dropped; the rest of the
  conversation plays over the combat camera, which follows the leader and turns with the sticks. Seen on
  702KOR when the PC drops during the Sion fight: `a_kreia_end` re-leads the PC, a `CancelCombat` clears
  the app's combat mode while the client still names the companion as player, and Sion's first node
  (`a_sion_end`, `SetPartyLeader(-1)`) sets it back to 1 from the PC's stale combat bit.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** while the dialog camera is installed, `SetCombatMode` records the
  mode and leaves the camera alone; `CGuiInGame::CloseDialog`'s `RestoreCamera` picks the combat or
  normal camera from it when the conversation ends.

## `CSWSArea::GetRoomForceRating` / `GetRoomDisableWeather` index the per-room tables with a `-1` room

- **Where:** `CSWSArea::GetRoomForceRating` @0x59fb3c and `CSWSArea::GetRoomDisableWeather` @0x59fc2c.
  Both call `GetRoomIndex(v, 0)`, which returns **-1** when the point is inside no room's walkable
  mesh, and then index `m_paRoomTableIndex[nRoom]` with no `nRoom < 0` test. The orig's no-room path is
  the three instructions `mov x21, #-1` (0x59fbe8) / `ldrb w8, [x8, x21]` (0x59fbf4, `x8` =
  `m_paRoomTableIndex` at +0x398) / `ldr w0, [x9, x8, lsl #2]` (0x59fbf8) — i.e. it reads the byte
  BEFORE the table and then indexes the rating table with **that** byte, so the first read is a 1-byte
  heap under-read and the second is an arbitrary-distance read through the heap word (an allocator
  header byte) that precedes the array.
- **Impact:** the return value is garbage — a force-power list gated on nothing in particular
  (rating 0 means "no restriction", so the usual outcome is *more* powers than the room should allow,
  or a wild rating). Reached from `CSWCCreature::GetSelfForcePowers` (0x2f7108) via
  `GetPersonalActions` on every action-bar repopulation, whenever the creature's position is in no
  room mesh. Measured trigger (v2212): the **area-transition window** — the new area's tables are
  loaded and the menus repopulate while the creature's `m_position` still holds the PREVIOUS module's
  coordinates (with the guard in place, a full `warp 701kor` entry plus a walk produced no
  `nRoom < 0` call at all, so it is the race and not the standing spot that does it).
- **Confirmation:** both rows are byte-exact **MATCH** rows in `tools/match.py`, and the orig's
  instruction order above is the whole proof. Found by playing the host build under ASan
  (v2212, warp tour, `701kor`): `heap-buffer-overflow ... 1 bytes before 4-byte region`, i.e. the
  `unsigned char[nRoomCount]` table with `nRoomCount == 4` (report in
  `docs/asan/roomforce_701kor.txt`).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both functions return 0 when `nRoom < 0` and report it once
  through `k2_fix_room_index_missing` (`platforms/compat/engine_fix_log.cpp`).

## K1: a creature's idle fidget queues a pointer to a stack local as its fire-and-forget payload

- **Where:** KOTOR 1 `CSWCCreature::AIUpdate` @0x263850 (libKOTOR.so): the idle-fidget picker stores the
  chosen animation in a stack slot and passes that slot's address as the payload (`strh w1,[sp,#8]`,
  `add x2,sp,#8`, then the virtual `AddFireForgetAnimation`). `InsertFireForgetAnimation` keeps the raw
  pointer, and `CSWCCreature::AnimateFireAndForget` later reads it as a `CSWCCombatAttackData`
  (`GetGameObject(pData->m_weaponObjectId)`) long after `AIUpdate` returned.
- **Impact:** a read of dead stack memory every fidget; in the shipped game the stale "weapon id" only
  feeds a discarded object lookup, so nothing visible. The K1 host ASan build stops on it
  (stack-buffer-overflow in `AnimateFireAndForget`). KOTOR II passes a zeroed static
  `CSWCCombatAttackData` instead — the same fix.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** K1 uses K2's zeroed static record; the matching K1 build keeps `&anim`.

## K1: `IosLocalizedStrings`' line reader runs `strlen` past its unterminated read buffer

- **Where:** KOTOR 1 `IosLocalizedStringsImpl::IosLocalizedStringsImpl(const char*, bool)` (port code,
  `src/port/ios/IosLocalizedStrings.cpp`): each pass reads up to 0x200 bytes into a stack `char[512]` and
  builds `std::string(szBuffer)` from it, so the length is wherever the next NUL happens to be — past the
  bytes read, through the rest of the frame.
- **Impact:** an out-of-bounds stack read on every pass (the K1 host ASan build stops on it during a save,
  which loads the save-name strings). A stale `'\n'` beyond `nRead` (left by an earlier, longer pass) can
  also turn leftover bytes into a bogus extra line near the end of a file.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the string is built from exactly the `nRead` bytes read.

## K1: the first bark bubble of a session is measured at a stale label width and drops its top lines

- **Where:** KOTOR 1 `CSWGuiBarkBubble::SetBark` @0x303d84 (libKOTOR.so). The ctor's
  `ScaleExtentForResolution` scales `LBL_BARKTEXT` twice (the panel's virtual `SetExtent` insets it,
  then the control loop scales the inset again by `g_SDL_mode.h / 768`), and K1's `SetBark` measures the
  text (`GetIdealHeight`) at that width. KOTOR II's `SetBark` opens with `SetExtent(m_Unk0x260)`
  (libkotor2 0x3c50a4); K1's does not. `Draw` then sets the real rectangle, the text re-wraps narrower
  and needs more lines than the bubble was sized for, and the first lines are cut. Later barks inherit
  the width the previous `Draw` left, so only the first one is affected.
- **Impact:** with a back buffer taller than ~832 px the first one-liner of a session (Endar Spire's
  "[Security Lockdown. ...]" door barks) shows only its last line. The shipped APK caps its buffer
  (`resolutionCap`: 720/640/480 unless "High Resolution" is on), so there it only comes out too tall;
  the ports run uncapped (any 1080p phone, a desktop window taller than ~832 px).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `SetBark` starts with K2's `SetExtent(m_Unk0x260)`.

## K1: closing the in-game menu plays its close sound twice

- **Where:** KOTOR 1 `CExoSoundSourceInternal::Resume` @0x681258 (libKOTOR.so) replays the source on its
  live 3D (else 2D) voice without asking whether it was ever paused. `CGuiInGame::HideSWInGameGui`
  plays GUI sound 5 (@0x241d14) and then calls `SetSoundMode(GENERIC)` (@0x241d30), whose
  `ResumeAllSounds` (@0x67fdf0) calls `Resume` on every source -- including the close sound that just
  started, which `PlaySourceOn2DVoice` (@0x6812ac) then starts a second time. KOTOR II's `Resume` is
  empty. Traced under lldb: the same source goes through `PlaySourceOn2DVoice` twice, once from
  `HideSWInGameGui` and once from `SetSoundMode -> ResumeAllSounds -> Resume`.
- **Impact:** every menu close (Start, B, Esc) doubles its sound, in the shipped Android APK too.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `Resume` returns early for a source that is still playing
  (`IsPlaying()` is false for a paused or finished channel, so paused sounds still resume).

## K1: the target reticles are centred for their 768-line size, then drawn at the screen's

- **Where:** KOTOR 1 `CSWGuiMainInterface::UpdateIndicator` @0x36c0b8 and `UpdateStickyTarget` @0x36dfa0
  (libKOTOR.so). The reticle rectangle is placed at the target's reticle point minus HALF its size in
  768-line units (`vPos - nSize / 2`), and only then sized `g_SDL_mode.h / 768 * nSize`.
- **Impact:** the ring sits `(h / 768 - 1) * nSize / 2` right of and below the head: ~2 px at the
  APK's 720-line cap, ~13 px for a close target at 1080 lines (High Resolution, the host builds).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both centre on the scaled size.

## K1: bark bubbles with a speaker play their voice-over at the world origin (often silent)

- **Where:** KOTOR 1 `CExoStreamingSoundSourceInternal::InitializeSource` @0x682188 (libKOTOR.so) passes
  the source's `m_b3D` (`[x19,#0x8c]`, read at 0x6827c4 / 0x682d3c / 0x682de4) to
  `FModAudioSystem::CreateStream`, which opens the stream `FMOD_3D` (libandroid_port.so @0x8c404, mode
  0x08000091). `CSWGuiBarkBubble::SetBark` marks a bark's stream 3D whenever the bark has a speaker.
  Nothing in K1 ever positions a stream: its `Update3DStream` sets only volume and pan (K2's also sends
  the position), and the one `SetChannel3DPosition` caller, `PlaySourceOn3DVoice` @0x67e94c, handles
  one-shots.
- **Impact:** the voice plays at (0,0,0); ~56 m from the listener on the Endar Spire the rolloff is
  about -35 dB, so the bubble shows its text in silence. Speakerless barks and conversation VO are 2D
  and play. Positional looping ambients and 3D one-shot streams were muted the same way.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** K1 streams are always created 2D; the engine's own distance
  volume and pan (`Update3DStream`) then apply as intended. The fix is in the port's `CreateStream`
  (src/port/fmod/FModAudioSystem.cpp), not in libKOTOR.so's byte-matched code.

## K1: leaving Advanced Graphics Options switches High quality graphics off

- **Where:** KOTOR 1 `CSWGuiOptionsGraphicsAdvanced::SetUpOptions` @0x37f3e8 (libKOTOR.so). K1's
  `AurCheckFrameBufferModificationsAvailable` always answers 0, so the page greys `CB_FRAMEBUFF` and sets
  `m_bFrameBuffer = 0`. Closing the page (`HandleInputEvent` back arm -> `SetNewOptions` @0x37f888) writes
  that 0 through `CClientOptions::SetFrameBuffer` -> `AurDisableFrameBufferEffects`, which in K1 sets
  `preferPerfToVisual = 1`: the Graphics page's "High quality graphics" / "Frame Buffer Effects" pair.
- **Impact:** opening Advanced Graphics Options and backing out (or OK) turns performance mode on: Frame
  Buffer Effects off, Shadows off and disabled, Brightness disabled. The shipped Android APK shows the
  Advanced Options button too.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** with the checkbox unavailable, `m_bFrameBuffer` carries the
  current `preferPerfToVisual` instead of 0, so closing the page leaves it as it was.

## K1: New Game is silent (character generation, and the main menu after Cancel)

- **Where:** KOTOR 1 `CSWGuiClassSelection::CSWGuiClassSelection` @0x3046a4 (libKOTOR.so) shows the
  "classsel" load screen with music on (`ShowLoadScreen(1, 1, 0)`), so `StartLoadMusic` @0x21a3c8 calls
  `StopMenuMusic`, which deletes the menu stream. `OnPanelAdded` then asks for the chargen theme
  (`StartMenuMusic(0, ...)` @0x3066a0, mus_theme_rep), but `StartMenuMusic` @0x210bf8 only builds a
  stream when one exists with another file or when `m_bStartMenuMusicOnMovieEnd` is up, and the menu's
  own start already cleared that flag. KOTOR II plays the module's load music on this screen instead;
  K1 sets no load music, so nothing plays. Cancel -> `DisplayMainMenu` -> `StartMenuMusic(1, ...)` hits
  the same null stream, so the menu stays silent too. All four functions are byte-identical to ours.
- **Impact:** no music from New Game until the first module's ambient music.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `OnPanelAdded` raises the flag (`SetStartMenuMusicOnMovieEnd(1)`)
  before asking for the theme; the theme's stream then also lets Cancel switch back to the menu music.

## Class selection with a controller: the first tile slides forward with no caption or description

- **Where:** KOTOR II `CSWGuiClassSelection::CSWGuiClassSelection` @0x37c02c ends with
  `SetActiveControl(&m_aOptions[0].m_button, 0)` (vtable slot 0x18 at 0x37ceb8). With a pad plugged in,
  the button's gain-focus event runs `OnEnterButton` -> `OnSelectButton`, which slides tile 0 (Male Jedi
  Consular) forward and fills `LBL_CLASS` / `LBL_DESC`. `OnPanelAdded` then blanks both labels and asks
  for the same control again, and `CSWGuiPanel::SetActiveControl` returns early when the control is
  already active, so nothing describes it. The ctor, `OnEnterButton`, `OnPanelAdded` and
  `SetActiveControl` are all byte-identical to ours. KOTOR 1's ctor has no `SetActiveControl`, so its
  `OnPanelAdded` call is the first one and fills the labels.
- **Impact:** the Consular tile is forward but the caption and description are empty until the
  player moves away and back.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `OnPanelAdded` resets the slide states and clears the active
  control before activating tile 0, so the gain-focus event slides it forward and describes it again.

## K1: the action queue crashes when an equip/attack node is queued between combat-round actions

- **Where:** KOTOR 1 `CSWGuiMainInterface::UpdateActionQueue` @0x36ce14 (libKOTOR.so) passes a null
  `CSWSCombatRoundAction*` to `GetActionIcon` (@0x36d118, `mov x3, xzr`) for a queued node of type 12, 15
  or 46 when the combat round has no current action. `GetActionIcon` @0x36d9e8 reads the type of the round
  action for a type-12 node with no null test (0x36da78 `ldrb w8, [x21, #0x10]`). KOTOR II's
  `GetActionIcon` adds the test (0x432dd4 `cbz x21`), so Obsidian fixed this.
- **Impact:** SIGSEGV (fault address 0x10) from the HUD draw. Seen once on the Thor while equipping a
  lightsaber during combat (`platforms/tombstones/bastila_saber_tombstone_00`, local). Timing dependent.
  It did not happen again when retried.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** K1 gets K2's test (no icon for that slot).

## `DoPassiveSelection` — the combat fallback targets a non-hostile object it never checked it can see

- **Where:** `CClientExoAppInternal::DoPassiveSelection` (K1 APK @0x215780), the combat-mode arm
  after the last-target re-acquire. It walks `listInFront` (the front-cone list `GetNearestObjects`
  builds) for a hostile the player `CanSee`, then the visible table for one; failing both it targets
  `pBest`, the first NON-hostile object of the cone, and that one is the only pick in the function
  with no `CanSee` test.
- **Impact:** when combat ends (the last hostile dies while combat mode is still up) the reticle
  jumps to a footlocker or a door in another room, through the wall, until combat mode drops and the
  ordinary scan (which tests `CanSee`) takes over. K1 is the visible case: its non-hostile radius is
  30 m (`GetNearestObjects(30, 30)`, the APK and the Switch alike); KOTOR II's 10 m rarely leaves
  the room.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `pBest` is only taken when `CanSee` (cached in the visible
  table like the other picks) says so.

## `MainLoop` — an "enemy sighted" pause parked during combat never clears, and stops every later one

- **Where:** `CClientExoAppInternal::MainLoop` (K1 @0x214420..0x21445c) and `SetAutoPaused`. Opening a
  door calls `DelayNextEnemyAutoPause` (5 s). An enemy sighted inside that window does not pause:
  `SetAutoPaused` parks the level in `m_Unk0x53c` (K1 +0x534) and arms a 1 s timer. When the timer
  runs out, `MainLoop` applies the pause and resets `m_Unk0x53c` to 0xff, but only if
  `m_combatMode == 0`; with combat already under way (the common case behind a door) it skips both.
- **Impact:** `DoPassiveSelection` only sights an enemy (the auto-pause, the tutorial) while
  `m_Unk0x53c == 0xff`, so no "enemy sighted" pause fires again until `UnloadModule` (a module change
  or a load). KOTOR II has the same code.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a timer that expires in combat drops the parked level.

## Conversations whose automatic camera angles lock into wide shots never frame an interjecting speaker

- **Where:** `CSWSDialog::InitializePsuedoRandomCameraAngles` (K1 @0x466b30) seeds the automatic-angle
  rotation (`m_anCameraAngles`, 19 entries) from a hash of the DLG resref:
  `hash += c + c * (int)pow(10, i)`, start `hash % 19`, step `hash % 5 + 1`, direction `hash & 1`. The
  rotation (`AssignPseudoRandomCameraAngle`, K1 @0x4695fc, also inlined into both `SendDialogEntryNode`
  and `SendDialogReplyNode`) advances once per entry AND once per reply list.
  `CSWBehaviorCameraDialog::SetShot` (K1 @0x2fa518) returns at once when a wide shot (type 3) follows a
  wide shot.
- **Impact:** a step of 5 going backwards cycles indices 18, 13, 8, 3 (angles 1, 3, 1, 3). With the
  reply lists taking the 1s, every CameraAngle-0 entry gets a 3. When the player mashes through
  single-reply "continue" nodes, no player shot happens in between. The camera then stays on the
  conversation's first wide shot, and party interjections are never framed. `(int)pow(10, i)`
  overflows for resrefs over 10 characters: arm64 `fcvtzs` clamps it to `INT_MAX` while x86 PC gets
  `INT_MIN`. Android and PC therefore pick different conversations for this. `dan13_juhani`
  (Dantooine, Juhani; Mission and Canderous interject) locks on arm64 (start 0, step 5, backwards) but
  not on PC (start 6, step 5, forwards).
- **Confirmation:** reproduced on the K1 host build with a Thor save. lldb shows the interjections'
  `SetShot` calls taking the early return. K1's `libKOTOR.so` has the same seed code (`fcvtzs w`),
  table, stepping and guard, so the stock Android game does this too. KOTOR II has the same code.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a wide shot after a wide shot is skipped only when it frames
  the same two participants (in either order), so a new speaker is always framed. A wide shot of
  participants standing far apart still frames them from a distance, as the original does.

## K1: a directory listing on disk builds each name from the whole `d_name` buffer (the Movies page lists only Credits)

- **Where:** KOTOR 1 `CExoBaseInternal::GetDirectoryList` @0x6456c8 (libKOTOR.so), the `readdir` arm:
  each entry's name is `std::string(d_name, sizeof(d_name))` (0x645c84: a 272-byte allocation, then
  16-byte copies from `dirent + 19`), so the string carries every byte after the name's terminator.
  The extension filter then takes `rfind('.')` over that whole buffer.
- **Impact:** wherever the bytes after a name hold a `.` (a stale longer name, or, on macOS's 1024-byte
  `d_name` in packed records, the following entries' names), the "extension" is a later file's and a
  matching file is dropped; on macOS the copy also reads past the record. Android K1 reads its movies
  from the OBB, so the arm rarely runs there; on a desktop install the movies are loose files beside
  their `.txt` subtitles, and the Movies page found none of the watched ones.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the name is `d_name` up to its terminator.

## K1: a directory in the patch OBB hides the main OBB's directory of the same name (watched movies missing)

- **Where:** KOTOR 1 `CExoBaseInternal::GetDirectoryList` @0x6456c8 (libKOTOR.so): with `c == 0` it lists
  the patch OBB's entry for the directory if there is one, and the main OBB's only when the patch has
  none. The Android port split the data across two archives; the Win32 game it came from listed one
  directory on disk.
- **Impact:** the patch OBB has its own `movies/`, so the Movies page never lists a movie that only the
  main OBB carries (`01a` the intro, `02`, `09`, `05_3a`, `05_8a` ...), watched or not; the same holds
  for any other directory both archives have.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both archives' entries are listed, the patch's winning a name
  both have.

## Uninitialised locals that reach a result (the `-Wall` pass over the Windows builds)

- **Where:** read by a `-Wsometimes-uninitialized` / `-Wuninitialized` pass over the x86-64 clang
  builds (`tools/wallscreen.py`). The transcriptions keep the original's shape: the arm64 binary leaves
  whatever a register held, and the faithful spelling is an uninitialised local. An x86-64 clang may
  read different garbage, or (as `HitCheckMouse` showed) treat the path as undefined and drop it.
- **Sites, each with the value it now gets:**
  - `CGameObjectArray::AddObjectAtPos`: inserting at the tail of a bucket chain returned the
    uninitialised status (the original returns the list bit: 1, "failed", for a character id). 0.
  - `CClientMsgSysAdmin::GetTokenPair` and its server twin in nwshandleserveradminmessage.cpp: a
    0-byte message branched on the unread delimiter. 0.
  - `CSWCCreatureStats[_ClassInfo]::GetMinSpellRange`: an unknown range code returned garbage. 0.0f.
  - `CSWMGGunBank::SetGunModel`: a gun whose behaviour is not fire / track-and-fire, or is not
    attached, used an uninitialised (on arm64, the previous iteration's) model. It now keeps the old
    gun.
  - K1 `CSWMiniGame::AddObstacle`: a model that fails to load returned a garbage obstacle. 0, as K2.
  - `CSWGuiStore` (K1) / `CSWGuiCreateItem` / `CSWGuiCreateMedicalItem` `PopulateInventoryItemListBox`:
    no player creature walked a garbage repository. Returns.
  - `CSWSCreature::AddItemCastSpellActions`: an item with no active
    property looked up a garbage spell id. Returns 0.
  - `AIActionExamineMine` / `AIActionFlagMine` / `AIActionDisarmMine`: the own-mine and
    not-trap/door/placeable arms put a garbage DC in the feedback message; the disarm arm compared it
    to decide whether the trap fires. 0.
  - `CSWSCreature::WalkUpdateLocation`: an animation other than walk/run read a garbage rate. 0 (no
    step).
  - `CSWSPlaceable::ApplyComputerSpikeBonus` (no spike item: garbage bonus) and
    `CSWSPlaceable::EventHandler`'s attacked event with no attack data (`pAttacker` garbage, then
    dereferenced). 0.
  - `CSWSMessage::WriteGameObjUpdate_MinorGUIPanels`: always returned garbage. 0.
  - `CServerExoApp::TestDirectLine`: no module left the area pointer garbage (K1 then dereferenced
    it). 0.
  - VM handlers that push a garbage value or return a garbage status when their object is missing:
    `SoundObjectGetVolume` / `GetFixedVariance` / `GetPitchVariation`, `GetSubRace`,
    `GetBonusForcePoints`, `SetItemNonEquippable`, `SoundObjectSetPosition`, `SoundObjectFadeAndStop`,
    `ChangeObjectAppearance` (a garbage status aborts the script), and K1's `GetMinOneHP`,
    `GetCommandable`, `GetEncounterData` and `GetIsRacialTypePlayable` (which indexes the race table
    with it). 0, or K2's initialiser.
  - `ExecuteCommandPlayAnimation`: a script not run on an object called through a garbage
    `pObject`. Returns.
  - `ExecuteCommandGetInPersistentObject` on an encounter with a nonzero zone: garbage scan start
    and cursor. Answers OBJECT_INVALID.
  - K1 `CSWSPlayerLUOInventory::ItemListGetItemObjectID` / `ItemListGetItem`: a bad list selector.
    0, as K2.
  - `CAuroraProcessedTexture::GetTextureSize` with no resource, `CRIMFile::WriteResource`'s name/extension
    lengths of a null string. 0.
- **Fixed under `K2_FIX_ENGINE_BUGS`** at each site. The by-design rows the pass still reports (an
  argument the callee never reads, or a path the caller rules out) are listed in `docs/TOOLS.md`
  `wallscreen.py`.

## Format, delete and string-literal slips the same `-Wall` pass found

- **Where / impact:**
  - K1 `CClientInput` capture-event playback scans the 32-bit message size with `%ld` (an 8-byte
    store into a 4-byte local on LP64 macOS/Linux) and the object id with `%lx` (K2: `%x`) into a
    64-bit local (4 bytes stored on Windows / always, the upper half left uninitialised).
  - Both `ImageWriteTGA` overloads (and K1's `ImageFlipY`) and the compressed-resource read in
    `CExoResFile` free `new[]` buffers with scalar `delete`.
  - The `converttobinary` console command tests `GetToken`'s result against a `""` literal; a missing
    argument is a null token, so it reaches `FindFirstFileA(NULL)`.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `%d` / `%llx`, `delete[]`, and a null test.

## A save that cannot clear its slot leaves every sound paused (music off, conversations skip)

- **Where:** both games' `CServerExoAppInternal::SaveGame` push `SetSoundMode(PAUSE_ALL)` first, and
  the pop is `StallEventSaveGame`'s `SetSoundMode(GENERIC)`. The two early `return 0`s (not enough
  disk space; `CleanDirectory` of an existing slot fails) never reach the stall event, so the mode
  stays pushed with `g_bSoundPaused` set.
- **Impact:** the music stops (it comes back when the menu's own `GENERIC` pops one level), and every
  conversation VO stream is paused the moment `CExoStreamingSoundSourceInternal::Play` starts it
  (`g_bSoundPaused && GetPauseable`), so `IsSoundPlayingInDialog` reads false and each line advances
  at once. Reproduced on k1-macos-pc by overwriting a save whose folder is read-only.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** each early return pops the mode (`SetSoundMode(GENERIC)`).

## K1: the main menu leaves every save's savenfo.res open

- **Where:** KOTOR 1 `CSWGuiMainMenu::FindNewestSaveGame` @0x375bd0 (libKOTOR.so) `SDL_RWFromFile`s each
  save's `savenfo` to `fstat` it and never closes it.
- **Impact:** one leaked handle per save per call. Harmless on Android and POSIX; on Windows a file
  that is open cannot be deleted, so overwriting any of those saves fails in `CleanDirectory`
  (`unlink ... savenfo.res: Device or resource busy`) -- which then trips the entry above.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `SDL_RWclose`.

## K1: resuming a paused sound leaks its channel record

- **Where:** KOTOR 1 `CExoSoundSourceInternal::Resume` @0x681258 replays the source through
  `PlaySourceOn3DVoice` / `PlaySourceOn2DVoice`, which take a NEW channel record from
  `FModAudioSystem::PlaySound` and overwrite the voice's channel id. The paused record is never
  stopped, and `libandroid_port.so`'s records are only reclaimed by `StopChannel` (`CleanChannels`
  runs at shutdown only).
- **Impact:** every pause/resume of a playing 2D/3D source (any menu, a save) holds one of the
  `n2D + n3D + 5` records for good; when they run out `PlaySound` / `PlayStream` answer -1 and sounds
  and VO stop playing.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `Resume` stops the old channel before it replays.

## Mouse look: the camera's tilt (and free look's turn) shrinks as the frame rate rises (K1 + K2)

- **Where:** `CClientExoAppInternal::UpdateCamera` hands `CSWCModule::TiltCamera` the mouse delta
  (`GetMouseDelta`: the mickies moved since the last frame over `fMaxMickies`), and free look
  (camera mode 5) gets `TurnCamera(delta, fFrameTime)`. The consumers then multiply by the frame
  time as if the value were a stick's rate: `CSWCameraOnAStick::Control_SetCameraOrientation`
  (`m_fUnk128 += rate * dt * m_fTiltAccel`; K1 `fTiltAccel * m_fTiltSpeed * dt`),
  `CSWCameraFreeLook::Control` (`m_fTiltSpeedScale * dt * m_fTiltSpeed`) and `TurnCamera`'s mode-5
  arm (`fHalf * fTurnSpeed * fParam2`). The frame time is counted twice: the same mouse travel tilts
  1/n as far at n times the frame rate (2.4x less at 144 fps than at the 60 fps lock). The on-a-stick
  and combat cameras' turn uses the delta per frame directly, so turn and tilt drift apart.
- **Impact:** K2's vertical mouse look (pc UI, and free look on every build) gets sluggish on a
  high-refresh display or with the framerate lock off. K1's on-a-stick camera clamps its pitch to the
  style's TILTUP/TILTDOWN (both 0 in Taris's Upper City: the mouse cannot tilt it at all), so in K1
  the fix shows only where a style allows tilt, and in free look.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the mouse value is taken at its size for a 60 fps frame:
  the tilt is scaled by (1/60) / the pause timer's frame time, and free look's turn gets 1/60 s for
  its time (0 while the world timer stands, as before). At 60 fps nothing changes.

## Turret minigame: mouse aim slows as the frame rate rises (K2)

- **Where:** `CClientExoAppInternal::ProcessInput`'s Type 2 (turret) minigame arm passes
  `PollInput(0x17/0x18) / g_fMouseSensitivity` -- the mickies accumulated since the last poll, a
  per-frame distance -- to `CSWMiniGame::UpdateMouse`, which multiplies by
  `m_movementPerSecond * fDelta`. The frame time is counted twice: the gun turns 1/n as far for the
  same mouse travel at n times the frame rate, and the +-1 clamp (20 mickies a frame) moves with it.
  The keys and pads (`UpdateAxis` / `UpdateJoystick`) are rates and are fine; the touch port replaces
  the pair with `GetIosControl`.
- **Impact:** 107PER, 421DXN and 505OND turret fights aim 2.4x slower with the mouse at 144 fps than at 60.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the pair is scaled by (1/60) / fDelta before the clamp, so
  it is the 60 fps rate at any frame rate.

## K2 camera look limits ease back 3 degrees a frame

- **Where:** `CClientExoAppInternal::ProcessInput` (K2 driving arm): once the mouse look ends,
  `CameraLookUp` / `CameraLookDown` (the on-a-stick camera's pitch limits) step back toward the
  style's TILTUP / TILTDOWN by a fixed 3.0 a frame (so does the pc UI's both-buttons walk, toward 0).
- **Impact:** the camera's pitch snaps back to its range 2.4x faster at 144 fps than at 60, about
  12x with the lock off at several hundred fps.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the step is 3 x 60 x the pause timer's frame time.

## K1 vs K2 variant scan (2026-10-20): K1 branches missing a reset, check or cleanup K2 has

Every row is ORIGINAL K1 behaviour (each read against `libKOTOR.so` with `tools/dumpfn.py --k1` /
`--pair`). All are fixed under `K2_FIX_ENGINE_BUGS` in the K1 arm; the matching build is unchanged
(`datadefs.py --verify`: code-identical, both games).

- **Stopping a sound leaves its loop playing; finished sounds leak channel records.** K1
  `CExoSoundSourceInternal::Release` @0x68074c and `ShutDownSource` @0x6808a0 set the voice's channel
  to -1 with no `StopChannel` (K2 stops it). `FModAudioSystem::ReleaseSound` stops the channels only
  when the wav's last reference goes, so stopping one of several emitters sharing a wav (ambient loops)
  leaves its loop running, and a reaped one-shot whose wav is still loaded holds its channel record
  until the pool runs dry (new sounds and VO answer -1: the same failure as the `Resume` leak above).
  Fixed: both stop the channel first, as K2 does.
- **`CSWGuiStore::PopulateStoreItemListBox` (K1 @0x3ad65c) loops until it has placed as many rows as
  the store holds, but skips null and plot items.** One plot/null item: the largest size group is
  listed again; only plot/null items: the store hangs on open. Fixed: K2's valid-item pre-count.
- **`CSWGuiMainMenu::OnNewGamePicked` (K1 @0x374240) never resets the tutorial tracker** (K2 calls
  `ResetTutorialWindowTracker`): a New Game after playing in the same session never shows the tutorial
  boxes already seen, and saves them as seen. Fixed: the K1 6-byte `m_tutorialWindowShown` is cleared.
- **`CSWGuiSaveLoad::LoadGame` (K1 @0x39e740) returns with `g_UI_mutex` still locked** when a save
  needs a missing LIVE package. Fixed: unlocked on that return.
- **`CSWSDialog::GetSpeaker` (K1 @0x4676fc) trusts a cached speaker id** without `GetGameObject`
  (K2 re-resolves the tag): a speaker destroyed and re-spawned under the same tag mid-conversation
  gets no lines. Fixed: K2's arm.
- **`CSWSCreature::ResolveDefensiveEffects` (K1 @0x442db0) ADDS the blaster-deflection DECREASE
  effect's amount.** An item with the penalty property makes its wearer better at deflecting bolts.
  Fixed: K2's subtraction. (Both games: `EffectBlasterDeflectionIncrease/Decrease` store the amount in
  integer 0 and the resolver reads integer 1, so the scripted effects do nothing; not changed.)
- **`CSWPartyTable::KillNPCObject` (K1 @0x53943c) does not clear a pending future leader** naming the
  removed NPC, and K1 `CrownFutureLeader` @0x2147fc dereferences the null lookup. Fixed: K2's clear and
  K2's null test.
- **K1 `CopyModuleToCurrentGame` (@0x54418c) opens the module in the patch OBB, then overwrites the
  handle with the main OBB's.** One leaked `SDL_RWops` per module copy, and the main OBB's module wins.
  Fixed: the main OBB only when the patch has none.
- **K1 `MainLoop` runs `DoPassiveSelection` every other frame with one frame's delta**
  (0x2141ac): its timers (the 1 s out-of-sight target hold, the 10 s enemy-sighted and trigger
  latches) run at half speed. Fixed: the skipped frame's delta is carried into the next call.
- **K1 `ProcessInput`'s driving arm has no global-fade test** (K2 zeroes both axes while
  `GetGlobalFadeActive`): the player can walk during a scripted fade to black. Fixed: K2's test
  (`GetGlobalFadeActive`, K2-only, is compiled for K1 too under the flag).
- **K1 `CGuiInGame::UpdateShotCamera` (placeable shot) and `SetComputerCameraShot` never turn a video
  effect off**: a dialog line with no effect keeps the previous line's scan noise / tint. Fixed:
  `DisableVideoEffect` on those arms.
- **K1 `MainLoop`'s end-game return to the menu does not `DisableVideoEffect`** (K2 does). Fixed.
- **K1 `CClientOptions::SetDefaultGraphicsOptions` only stores the defaults**: Graphics > Default shows
  the Grass box at its default while grass keeps its old state. Fixed: `AurEnableGrass` /
  `AurDisableGrass` as K2 does (shadows are re-applied by K1's `SetUpOptions`).
- **K1 `ClearBuckets`/`AddPartToMeshBuckets`/`AddPartToCapBuckets` index fixed `Bucket[5000]` arrays
  by the raw GL texture name** (K1 @0x5fbd5c, no bound; K2 uses a map). A driver that hands out names
  past 4999 (a monotonic allocator, after many area loads) writes past the arrays into other .bss
  globals and the mesh is never drawn. Fixed: such a name goes in bucket 0, whose parts bind their own
  texture.
- **K1 `FindModel` writes "Names Differ" through a null `SDL_RWops`** when `AR_ERROR.LOG` cannot be
  opened (read-only working directory): a crash on a model whose internal name differs from its resref.
  Fixed: the null test.
- **K1 `Gob::LoadAddInAnimations` dereferences a missing stunt model** (K1 @0x5b8070). Fixed: returns
  false.
- **K1 `GLRender::RenderObjectsVTC` (the font path) leaves the normal array enabled** with the font VBO
  bound (K2 disables it): the same class as the `RenderObjectsVTCQuadUP` NVIDIA fault. Fixed.

## Frame-rate and K1 vs K2 follow-ups (2026-10-20, user decisions)

- **Particle emitters drop the fractional spawn time** (both games, `PartEmitter` accumulator spawn
  type, `auremitter.cpp`): every spawn reset the accumulator to 0, so the remainder below one
  particle was thrown away each time. An emitter whose birth rate lies between the frame rate and
  twice it lost up to half its particles at 144 fps. **Fixed under `K2_FIX_ENGINE_BUGS`:** only the
  spawned particles' share of the time is used up (a remainder of a whole interval or more still
  resets, as before).
- **Dangly meshes (hair, robes) run faster than real time above 80 fps** (`PartDanglyMesh::Animate`,
  `auranim.cpp`, the restored macOS-lineage solver): the step was clamped to [1/80, 1/28.6] s, and
  the clamped step was also handed to the child cascade (`Part::Animate`), so a pendant and its
  children ran 1.8x at 144 fps. The explicit Euler step scales with the step, so **under
  `K2_FIX_ENGINE_BUGS`** only the ceiling stays and the children get the real frame time.
- **K1 `CClientExoAppInternal::UpdateCamera` never stops the camera turn while input is inactive**
  (K2 calls `TurnCamera(0, 0)` when `g_pExoInput->GetActive() == 0`): a turn in progress when the
  window loses focus kept going. **Fixed under `K2_FIX_ENGINE_BUGS`** with K2's call.
- **Not fixed (user decision, "no for now"): K1 clamps bonus attacks when it stores them**
  (`nwseffectlist.cpp` `OnApplyModifyNumAttacks` / remove, ~773/795): with stacked +attack effects
  the smaller one's removal takes an attack the larger one still grants. A fix changes combat balance.
- **Not fixed (user decision, leave it): scripted blaster-deflection effects do nothing in both
  games** — `EffectBlasterDeflectionIncrease/Decrease` store the amount in integer 0 and
  `ResolveDefensiveEffects` reads integer 1. Original behaviour players are used to; left as shipped.
- **Not fixed (user decision, leave as original): a dialog skip pressed while the conversation is
  paused fires when it resumes** — K1 Mission's first line (`tar04_mission.dlg` entry 9, script
  `k_ptar_misrun`) pauses the conversation while she runs to the player; an A / Start / click during
  the run marks the line skipped, `CSWSObject::UpdateDialog` returns early while paused, and on resume
  `IsDialogDelay` stops the voice ("...But I can't") and shows the replies. The original K1 binary
  does the same; with no input the whole line plays.

## Found by the MSVC phase-4 triage (cl.exe `/analyze`, ASan on Windows, 2026-10-22)

The list with the dismissed rows is `docs/prep/msvc_phase4_triage.md`. Each of these reads a value
the original leaves to whatever a register or stack slot held; arm64/clang mostly happens to hold a
harmless value there, the x64 cl.exe build does not.

## `ExecuteCommandGetNearestObject` — the reputation criterion reads an uninitialised `pTarget`

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandGetNearestObject` (the
  `GetNearestCreatureToLocation` arm, 0xe2). The location arm never fetches `pTarget`; criterion 3
  (friend / enemy / neutral) still calls `GetReputation(pTarget->m_objectId)` (the original reads
  the never-written `[sp,#0xa0]` the same way, 0x57e690..).
- **Impact:** a script asking for the nearest hostile creature to a location dereferences stack junk.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `pTarget` starts null and a null target matches no
  reputation.

## `AIActionPhysicalAttacks` / `AddAttackActions` — a 2-byte feat id is queued as a 4-byte int parameter

- **Where:** both re-queue the attack with `AddActionToFront/AddAction(..., 1, &nFeatId, ...)` where
  the feat id is an `unsigned short` local or argument; tag 1 makes `AddActionNodeParameter` read 4
  bytes (ASan stack-buffer-overflow on Windows; the macOS ignorelist carries the same read,
  `platforms/asan_ignore.txt`). The reader of that node field (`m_attackFeatId`) is 16 bits, so the
  junk half is never used.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the value is widened into an `int` local first (K1 already
  does this in `AddAttackActions`).

## `HandleServerToPlayerActiveItemPropertyUpdate` — the uses-left loop runs to 64 over an 8-byte array

- **Where:** `CSWCMessage::HandleServerToPlayerActiveItemPropertyUpdate`. `1 << i` with i in 32..39
  wraps (both CPUs take the count mod 32) and re-tests the mask's bits, reading `nUsesLeft[32..39]`
  past the stack array and passing that to `SetItemPropertyUsesLeft(32..39, ...)`.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the loop stops at 8, the mask's width.

## `CSWCVisualEffectOnObject::UpdateMotionBlur` — state 0 applies an uninitialised effect id

- **Where:** 0x3fa8e0. The switch sets `nEffectId` for states 1..3 only; in state 0 the junk id
  reaches `Apply/RemoveHardCodedVisualEffect` (env map, self-illumination, light, alpha bands) and the
  speed-blur ratio is set every frame (w21 unset on the same path in the original).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** any other state returns.

## `CSWCCreature::ClientServerSyncHandler_Position` — `fScale` is unset for a non-walk, non-run animation

- **Where:** 0x491da4. `goto noColumn` skips the assignment; along a waypoint path the junk feeds the
  reference distance and so `m_Unk0x2b4`, the client's movement-animation speed (the original's s9 is
  unset on that path too). A scripted move with another animation gets a random speed per update.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** it starts at the walk rate (`m_Unk0x2c8`).

## `CSWGuiMainInterface::AddHelpPanel` (K2) — `pUnkB == "43"` compares string-literal addresses

- **Where:** the 4:3 test compares the pointer set from the literal `"43"` a hundred lines earlier
  with another `"43"` literal. It holds only when the compiler pools identical literals (cl `/GF`,
  on with `/O2`; clang at any level), so a cl.exe Debug build places the help panel at the
  widescreen offset on a 4:3 screen.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `strcmp`.

## `CWin32MSGQueue::ProcessSendQ` — the Unicode arm truncates the window procedure to 32 bits

- **Where:** `CallWindowProcW(reinterpret_cast<void*>(static_cast<int>(GetWindowLongW(hwnd, -4))))`
  (the ANSI arm next to it does not truncate). Any 64-bit host's code lies above 4 GB, so a sent
  Unicode message would call a cut pointer. No caller of `CWin32MSGQueue::Send` exists today.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** no `int` cast.

## `UpdateScenesAttach` — reads `m_nDieWithoutParent` from another event's payload

- **Where:** the attach behaviour's wildcard (`"*"`) callback. `Gob::DoEvent` hands it the event's
  payload when there is one, so for `ReportKey` it gets a `NewControllerKeyReport*` on the caller's
  stack instead of its `CAurBehaviorAttach*`, and reads report+0x34 before testing the event name
  (the macOS ignorelist's 0x791d48 entry; the cl.exe frame puts a redzone there: ASan
  stack-buffer-overflow on Windows, 211TEL). The value is discarded since the event is not
  `"Dying"`.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the event name is tested first.

## `CSWSEncounter::ReadEncounterScriptsFromGff` — a 16-character script resref is read as an unterminated string

- **Where:** each of the five script fields is `CExoString(ReadFieldCResRef(...).GetResRef())`.
  `GetResRef` returns the resref's 16-byte buffer, which has no terminator when the name fills it, so
  the string runs on into the stack (ASan stack-buffer-overflow, K1 KORR_M35AA / MANM26AA: a 23-byte
  `strlen`). The encounter then names a script with junk appended.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `GetResRefStr()` (the terminated copy).

## `CSWCollisionMesh::LoadMeshString` (K1) — a CRLF line end skips the `\n` without counting it

- **Where:** K1 0x54ebdc. After a `\r` the parser steps over a following `\n` (`str x10,[x1]`) but
  never decrements the remaining count, so a CRLF text walkmesh is read one byte further past its end
  per line, and the peek at `**ppData` itself can be past the end (ASan heap-buffer-overflow, K1
  MANM26AA placeables).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the peek checks the count and the skip decrements it.

## `CGuiInGame::DialogNodeSpanningTextParse` — the last segment reads past the text's terminator

- **Where:** after storing a segment that ended at the terminator (no `##` after it), the parser
  tests `buf[i + 1]` and `buf[i + 2]` before its own `!isDelim` exit, one and two bytes past the end
  of the copied text; a non-zero byte there sends it on parsing heap junk as further spans (ASan
  heap-buffer-overflow, K1 STUNT_06).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a segment that ended at the terminator finishes the parse.

## `CSWSEncounter::BuildInitialSpawnList` (K1) — two reads outside the spawn-point table decide what spawns

- **Where:** the K1 arm. The tier test reads `m_paSpawnPoints[j + 1]` when `j` is the last slot, and
  the group loop's first test reads `m_paSpawnPoints[-1].m_fPoints` (`nPick = -1`), both marked
  `sic` in the transcription. On the host that is heap junk (ASan heap-buffer-overflow 8 bytes before
  the table, K1 TAR_M03AB), and the junk decides whether a CR group spawns at all.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the last slot ends the tier, and the first test uses the
  group's first slot.

## Found by the warnings ledger (cl.exe `/W4 /analyze` + clang `-Wall -Wextra`, 2026-10-22)

Every bug-shaped warning row (uninitialised use, buffer over-read/overrun, null dereference, format,
precedence, delete) in `src/` was read against the original disassembly; the ledger and the per-row
verdicts are `docs/prep/msvc_phase4_triage.md` and `docs/prep/warn_bug_{0..3}.md`. Each entry below is
fixed under `K2_FIX_ENGINE_BUGS` (off in the matching build: `datadefs --verify all` 722/722
code-identical) unless it says otherwise.

## `CSWGuiCustomPanel::HandleInputEvent` / `CSWGuiMainInterface::HandleInputEvent` -- callbacks called with a never-set argument register
- **Where:** `CSWGuiCustomPanel::HandleInputEvent` K2 @0x376804 (key 0x29 -> `OnCancelPressed(pControl)`, x1 never set: it still holds `type`); `CSWGuiMainInterface::HandleInputEvent` K2 @0x426804 / K1 @0x35e47c (`DoPersonalAction` / `DoTargetAction(idx, nTarget)`, w2 never set, 0x42725c / 0x35eb6c).
- **Impact:** none observed -- no callee reads the argument and every other caller passes 0. Undefined behaviour only.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the locals are `nullptr` / `0`. No play-test needed.

## `CSWGuiInGameEquip::OnItemSelected` -- the slot-flag switch's default indexes past m_anUnk08
- **Where:** K2 @0x523e9c (`default: nSlot = 12`, `mov w8,#0xc`), K1 @0x338a04 (`default: nSlot = 10`); the arrays are `m_anUnk08[11]` / `[9]`.
- **Impact:** an unknown slot flag reads the next member (m_nUnk68/m_nUnk6c) as the item oid. The 11 (9) slot buttons never produce an unknown flag, so unreachable in practice.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** an out-of-range slot leaves m_nUnk70 0 (no cached item). No play-test needed.

## `CSWSCreature::GetSpikeDetails` -- a null repository item falls through the type filter
- **Where:** K2 @0x63eecc / K1 @0x423aa4: `if (pItem != 0 && base != 0x1b) continue;` lets `ItemListGetItem() == 0` (a stale oid in the list) reach `pItem->GetPropertyByTypeExists`.
- **Impact:** null dereference opening the spike/security list with a dangling repository entry.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** null items are skipped.

## K1 `CSWSCreature::GetTotalEffectBonus` -- damage decreases use the last increased type's flag
- **Where:** K1 @0x417eb8. `nFlag` is set only when a type has a positive increase; the decrease arm (`AddDamage(nFlag, -anDecrease[k])`) and the stacked-elemental immunity/resistance calls after the loop reuse it. With no earlier increase it was never set at all.
- **Impact:** a damage-decrease effect is recorded against the wrong damage type (or a junk one), and elemental stacked bonuses are resisted as the last increased type. K2 sets the flag every iteration (and passes the loop's final 0x4000 after it).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** K1 sets `nFlag = 1 << k` every iteration, K2's behaviour. **Play-test (K1):** a damage-decrease effect (e.g. a weakening power) on an attack; the floaty/combat log damage types.

## `CSWSMessage::HandlePlayerToServerGoldMessage` -- TransferGold through an unset creature
- **Where:** K2 @0x5d6bf4 / K1 @0x490538. `pCreature` is set only when the player's game object is a creature; `pCreature->TransferGold(...)` runs regardless.
- **Impact:** a gold message from a player whose object is not a creature calls through a junk pointer.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** null-initialised, the transfer is skipped.

## `CSWSMessage::HandlePlayerToServerParty` sub-op 10 -- the leave-party failure arm derefs a null self
- **Where:** K2 @0x5da298 / K1 @0x493db4. `if (pSelfCreature != 0 && ReplyToInvitation(...))` -- the else arm uses `pSelfCreature->m_objectId`, and `pCreature` (looked up from a wire oid) is never tested.
- **Impact:** null dereference on a leave-party message naming a missing member or sent by a non-creature player.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** either pointer null returns 1.

## `CSWSObject::LoadActionQueue` -- anAlloc unset for an unknown parameter type
- **Where:** K2 @0x642870 / K1 @0x4be624. `anAlloc[j]` is written only for types 1..5; the free loop switches on all of them.
- **Impact:** benign in practice (apValues[j] is 0 for those, so any stray delete is of null).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** zeroed with the other two arrays. A GFF "NumParams" above 13 overrunning all three 13-entry stack arrays is fixed too (below).

## `CSWSObject::AIActionDialogObject` -- party re-seat derefs a member with no server object
- **Where:** K2 @0x556b38 / K1 @0x4ca3e8. `pMemberGO != 0` guards only the ClearAllActions pair; `pMemberGO->m_objectId` goes to ComputeSafeLocation unguarded.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** such a member is skipped.

## `CSWSObject::AIActionLockObject` / `AIActionUnlockObject` -- skill feedback with a never-set DC
- **Where:** K2 @0x5597c0 / @0x558318, K1 @0x4ccdf8 / @0x4cbb60. A target that is neither a door nor a placeable leaves `nDC` unset and the Security feedback message broadcasts (and compares against) it.
- **Impact:** a junk DC / result code in the combat log for a script-queued lock/unlock on a creature or other object.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** nDC (and the roll/skill/item locals set under the same creature test) start at 0.

## `ExecuteCommandResistSpell` -- the resist feedback reports an unrolled d20
- **Where:** K2 @0x587e18 / K1 @0x52dc08. When DoSpellLevelAbsorption or DoSpellImmunity returns 1, `nRoll` is never rolled but the 0x11 feedback message carries it.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `nRoll = 0`.

## `ExecuteCommandIsEffectValid` -- duration read through a null effect
- **Where:** K2 @0x584df4 / K1 @0x52b964: `pEffect->m_durationTypeBits` after the `pEffect != 0` test.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a null effect takes the `nValue = 0` arm.

## Script handlers that read optional arguments they never popped (latent: the script compiler always supplies the defaults)
- **Where:** `ExecuteCommandVersusEffect` K2 @0x58ea58 / K1 @0x533bf0 (nLawChaos, nGoodEvil); K1 `ExecuteCommandGetReflexAdjustedDamage` @0x527b14 (nDC, 4-argument form; K2 initialises 0); K1 `ExecuteCommandShowPartySelection` @0x531ad4 (nForceNpc1/2 unless exactly 3 args); `ExecuteCommandGetInPersistentObject` K2 @0x575778 / K1 @0x51dc2c (nResidentObjectType, nPersistentZone; the existing gated zone check still read the unset zone); `ExecuteCommandSavingThrow` K2 @0x571640 / K1 @0x51986c (nSaveType).
- **Impact:** only reachable from hand-built bytecode with fewer arguments; reads stack garbage.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the NWScript defaults (ALIGNMENT_ALL 0, DC 0, -1 forced NPCs, OBJECT_TYPE_CREATURE 1 / PERSISTENT_ZONE_ACTIVE 0, SAVING_THROW_TYPE_NONE 0).

## K2 `ExecuteCommandGetNearestObject` -- class criterion reads m_classInfo[2] of a two-entry array
- **Where:** K2 @0x57df74, under `m_numClasses >= 3` (NWN's third class slot).
- **Impact:** dead (no KOTOR creature has three classes: `ReadStatsFromGff` clamps `m_numClasses` to 2);
  would read the member after the array (`m_classInfo[2].m_class` is byte +0x143 of the server stats,
  `m_nCHABase`). The matching (NDK, unfixed) compile reports `-Warray-bounds` on this clause, as it does on
  the `nwscreaturestats.cpp` third-slot reads below; that warning is this entry, not a transcription error.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the clause is dropped.

## `ExecuteCommandDoTouchAttack` -- feedback with never-set attack bonus and roll
- **Where:** K2 @0x58e2c0 / K1 @0x533504. A non-creature target (door/placeable arm) or non-creature caller sends the feedback message with `nAttackBonus` / `nRoll` unset.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both 0.

## `ExecuteCommandActionFollowObject` -- dereferences a null caller and an unchecked target
- **Where:** K2 @0x56289c / K1 @0x50bf1c. `pCreature->m_position` is read after `if (pCreature != 0)`; the followed object (`GetGameObject(oidFollow)->AsSWSObject()`) is never tested.
- **Impact:** crash when a non-creature runs ActionFollowObject or the target oid is stale.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns 0 when either is null.

## K1 `ExecuteCommandGetDamageDealtByType` -- pObject unset for a non-CSWSObject self
- **Where:** K1 @0x51b748 (K2 initialises it 0).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the existing `#if KOTOR1` uninitialised arm now also requires `!K2_FIX_ENGINE_BUGS`.

## `CSWRoomSurfaceMesh::LoadMeshText` -- aabb section before (or without) a faces section
- **Where:** `LoadMeshText` K2 @0x64f6e8 / K1 @0x554ca4 (ASCII walkmesh). `walkableMap` / `exteriorMap` are allocated by the faces section; the aabb reader indexes them unconditionally, and a short `sscanf` leaves `leafTri` unset.
- **Impact:** null dereference on a malformed text walkmesh (override content).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** leafTri starts -1 and the remap needs walkableMap.

## `Gob::SetAnimationTime` / `Gob::SetAnimationPercentage` -- writes through a stale pointer when the name is not playing
- **Where:** SetAnimationTime K2 @0x6cc624 / K1 @0x568db0, SetAnimationPercentage K2 @0x6cc6dc / K1 @0x568e60. The reverse name search falls out with `pAnimation` unset and stores the time through it: at 0x6cc690 x8 is the last entry's MdlAnimation* (or the name's first byte for an empty list), so `+0x14` of an animation header is overwritten.
- **Impact:** silent corruption of a model animation's header field (or a crash) whenever a script/GUI seeks an animation that is not on the play list.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** null-initialised; no match returns -1 as documented. **Play-test:** worth a look -- any visible change means a caller relied on the corruption.

## `PartSkin::PostProcess` -- zero-vertex skin mesh reads flags through null
- **Where:** PostProcess K2 @0x6f5c24 / K1 @0x605c94; K2 0x6f6304 `mov x8, xzr` then 0x6f6330 `ldrb w8, [x8, #0x54]` (K1 the same).
- **Impact:** crash loading a skin mesh whose vertex strategy has no vertices.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** no vertices reads flags 0 (every element offset off).

## `CResFNT::ReadColorMappedRLETGA` -- zero-byte buffer, full decode
- **Where:** K2 @0x796a44 / K1 @0x6439c8. `new unsigned char[m_fntSize]` (m_fntSize is never assigned, so 0) then `m_tgaSize` bytes decoded into it (already described in the function comment).
- **Impact:** heap overrun for any colour-mapped RLE .fnt (none ship, to my knowledge).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the buffer is m_tgaSize.

## K1 `AsyncPlayThreadProc` -- null argument falls through
- **Where:** K1 @0x65a310 (K2 @0x7b2134). K2's ExitThread is noreturn; K1's returns, so a null pArg continues into `pPlayer->m_hThread`.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `return;` after the ExitThread (dead in K2).

## K2 `CExoKeyTable::RebuildTable` -- pointer printed with %x
- **Where:** K2 only (`#if !KOTOR1`); the `#### Rebuild Table: %x %s` trace passes `m_paEntries` (a pointer) for %x.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `%p` with `static_cast<void*>`.

## `CExoResMan::GetNewResRef` -- unterminated digit buffer and a one-byte heap overrun
- **Where:** K2 @0x7c37a4 / K1 @0x66329c. `szNumber = calloc(nNumberLength, 1)`, `strncpy(..., nNumberLength)` (no terminator), `atoi(szNumber)` (reads past the block), then `sprintf(szNumber, "%d", nNumber)` writes the digits plus a NUL into it.
- **Impact:** for any resref that ends in digits (the common case: "p_bastila01"-style names when a new resref is minted), atoi may pick up trailing heap bytes and sprintf overruns the block by one byte.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** one byte more. No play-test needed beyond ASan.
- **Final pass (2026-10-22):** the one extra byte was not enough: the probe loop `sprintf`s counters up to "1000" into a buffer sized from the digit run (`calloc(nDigits)` K1 0x6633c0, or `calloc(nBaseLength ?: 3)` 0x663418), so any digit run under 4 (or a digit-less stem under 5) still overran. The gated arm now uses one 12-byte buffer, a checked calloc and a memcpy of the (at most 8) digits. No call site exists in src/ (exported only).

## `vformat` (AndroidJNI.cpp) -- realloc misuse and an overrun past the 999999-byte cap
- **Where:** `vformat` K2 @0x826338 (port/android/AndroidJNI.cpp). `pszBuf = realloc(pszBuf, ...)` leaks and then vsnprintf's into null on failure; when the cap ends the loop with the output still truncated, `pszBuf[n] = '\0'` writes past the nCap+1-byte block.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a failed realloc frees and returns "?", as does a still-truncated result.

## `CObjectLookupTable::GetUpdate` returns an unset pointer for a full ring
- **Where:** src/game/clientcore/ObjectLookupTable_ring.cpp `_ZN18CObjectLookupTable9GetUpdateEPvPj` (K1 0x20ae04, K2 0x35e63c). The `m_nCursorA == m_nCursorB` arm stores the 0x84 size and returns without writing x0, so the caller gets `this` back as the "buffer".
- **Impact:** none today; nothing in the tree calls GetUpdate.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns nullptr. No play-test.

## `nMenuArg` / `nUnused`: never-set arguments to callees that ignore them (harmless)
- **Where:** clientexoappinternal.cpp ShutDownToMainMenu and the two end-game arms of the main loop (`DisplayMainMenu(int)`, both games leave w1 unset); swguidialog.cpp `CSWGuiDialogComputer::SetReplies` (the base SetReplies' 4th argument, w4 clobbered by the preceding call).
- **Impact:** none; DisplayMainMenu passes it on to StartMenuMusic, which ignores it in K2, and K1 uses a literal; the base SetReplies does not read its 4th argument.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `nMenuArg = 0`, `nUnused = c`. No play-test.

## `CSWCCreature::SetPrecalculatedPosition` dereferences a missing server twin and a null pathfind info
- **Where:** src/game/clientcore/nwccreature.cpp `_ZN12CSWCCreature24SetPrecalculatedPositionERK6Vector` (K2 0x49ac20). `pServerCreature` is 0 when GetServerObject() or AsSWSCreature() is null, and the original then loads `pServerCreature->m_pPathfindInfo` (`ldr x8, [x20, #0x538]` at 0x49b04c with x20 = 0 from 0x49afc0). Further down, `pPathInfo` is null-tested before the `m_Unk038` store and then read unconditionally (`m_Unk014`).
- **Impact:** a crash moving a client creature that has no server twin. Not expected in single player.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns 1 (the "preempted" result) in both cases, both games. Play-test: walk the PC around; the gated paths should never fire.

## `CSWCCreature::GetSelfForcePowers` reads the area of a missing server twin
- **Where:** nwccreature.cpp `_ZN12CSWCCreature18GetSelfForcePowersER13CExoArrayListI21CSWGuiInterfaceActionE` (K2). The function handles a missing twin for the force points and item mask (`nHasServer = 0`), then calls `(GetServerObject() ? ...AsSWSCreature() : 0)->GetArea()` anyway.
- **Impact:** a null call when opening the self force-power menu of a creature without a server twin.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `pArea = nHasServer ? pServer->GetArea() : 0`. Play-test: open the self force-power menu.

## `CSWGuiPazaakStart::UpdateHelpText` indexes m_anCopies by an unbounded id
- **Where:** src/game/clientcore/swguipazaak.cpp `_ZN17CSWGuiPazaakStart14UpdateHelpTextEi` (K2 0x51373c): `ldr w11, [x0 + w1 sxtw*4 + 0xc628]` with w1 = nId, any value including -1 and ids >= 24.
- **Impact:** an out-of-range read inside the object, only when nStr is already -1. No visible effect.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** range-checks nId before the read. No play-test.

## `OnApplySetStateInternal` runs its feedback tail for a non-creature or a null effect
- **Where:** src/game/servercore/nwseffectlist.cpp `_ZN21CSWSEffectListHandler23OnApplySetStateInternalEP10CSWSObjectP11CGameEffecti` (K2 0x668218). `mov w23, #1` (bFeedback) and then `cbz x20` / `cbz x19` jump straight to the tail at 0x66840c. The tail reads `pEffect->m_creatorId`; if the caster exists, it stores the never-set nFeedbackState into the attack data and reads `pCreature->m_objectId` through null.
- **Impact:** a crash for a set-state effect on a non-creature target with a live caster, or for a null effect.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns 0 when there is no creature or no effect. No play-test needed (stun/paralysis effects on creatures are unchanged).

## `OnRemoveHitPointChangeWhenDying` dereferences a non-creature target
- **Where:** nwseffectlist.cpp `_ZN21CSWSEffectListHandler31OnRemoveHitPointChangeWhenDyingEP10CSWSObjectP11CGameEffect` (K2 0x6699b8). The `cbz x0` (AsSWSCreature null) skips only the PC bleed block; the death test after it dereferences the creature.
- **Impact:** a null read if the effect is ever removed from a non-creature.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns 1 when it is not a creature. No play-test.

## `OnApplyDisarm` schedules the weapon destroy with a null disarmer
- **Where:** nwseffectlist.cpp `_ZN21CSWSEffectListHandler13OnApplyDisarmEP10CSWSObjectP11CGameEffecti` (K2). The fallback arm, reached among other cases when the creator is not a live creature, passes `pDisarmer->m_objectId` (`ldr x3, [x20, #8]`, x20 = the creator lookup) when it destroys a non-droppable, destroyable weapon.
- **Impact:** a crash when a creator-less disarm (for example from a script) hits a creature holding a non-droppable, destroyable weapon.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** passes `pEffect->m_creatorId`, the id the lookup used. CSWSItem's destroy handler ignores the caller. Play-test optional.

## `CSWSPlaceable::EventHandler` (melee attacked) / trap trigger: two unchecked creatures
- **Where:** src/game/servercore/nwsplaceable.cpp. (1) `pAttacker` is null-tested for the party broadcast, then the weapon-slot switch reads `pAttacker->m_pInventory` untested. (2) In the trap trigger, `pCreature` is null-tested for the mine immunity, then the same-faction skip reads `pCreature->m_pStats->m_factionId` untested. Both carry existing "sic" notes in the source.
- **Impact:** a crash when a placeable is struck by, or its trap is triggered by, a caller that is not a live creature.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the switch runs only with an attacker; the faction skip needs a creature. No play-test (creature attacks and traps are unchanged).

## `CSWPartyTable` load trusts PT_NUM_MEMBERS / PT_NUM_PUPPETS
- **Where:** src/game/servercore/partytables.cpp, both LoadTable arms and the K2 puppet load. The count is clamped only to the GFF list length, then fills `m_partyMemberIndex[2]` / `m_puppetIndex[2]`.
- **Impact:** a save with more than two members or puppets writes past the arrays into the following party-table fields. The add/remove loops that the analyzer flagged then also run out of range.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** clamps both counts to the array size. Play-test: load any save (the counts are at most 2 in stock saves).

## `CResPLT` RLE decode overruns its output buffer
- **Where:** src/libsource/aurorainterface/resplt.cpp, the RLE loops in ReadPLT and ReadRLEPLT. A run's end, `dstPos + (ctrl & 0x7f) + 1`, is never clamped to m_nPLTSize.
- **Impact:** a malformed or truncated PLT writes up to 127 bytes past the heap buffer. PLT is a layered-texture format KOTOR's data probably never uses.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the run end is clamped to the buffer size; the over-read of the source is not bounded. No play-test.

## `CRIMRes::SetName` copies an unbounded name into a 16-byte slot
- **Where:** src/libsource/exoresman/rimfile.cpp `CRIMRes::SetName`. K2 does an unchecked memcpy of strlen bytes; K1 does the same through `__memcpy_chk`, which aborts.
- **Impact:** a stack/heap overwrite (K2) or an abort (K1) when a RIM entry is named with more than 16 characters. This is the RIM-writing path, not used by normal play.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the length is clamped to sizeof(m_szName). No play-test.

## `ImageDecode` writes 8 bytes per 4-byte pixel (and trusts its page/index counts)
- **Where:** src/libsource/AURORA/aurimage.cpp `ImageDecode` (K1 0x59ca58, K2 0x780ca0).
- **Impact:** the buffer is `new unsigned char[w*h*4]` but the original stores a 64-bit table read per pixel (`ldr x17` / `str x17, [x14], #8`, both games): w*h*8 bytes written, a w*h*4 heap overflow and the pixels at the wrong stride (looks like the LP64 port's `unsigned long`). The stack palette table is 15 pages (0x3c00 B) but nPageCount (up to 256) is not clamped and nIndex (16-bit) is not bounded against 0xf00. Reached via CAurTextureBasic::ConstructImage when m_panScheme is set (AurTextureSetLayered from CAurMaterial), the NWN-style layered/PLT path, probably unused by shipped KOTOR data.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `unsigned int*` output; nPageCount clamped to sizeof(aTable)/0x400; aTable zeroed; out-of-range nIndex gives 0. Play-test only if a layered texture is ever hit.

## `ImageReadS3TC` trusts the file's data-size word
- **Where:** src/libsource/AURORA/aurimage.cpp `ImageReadS3TC(FILE*, ...)`.
- **Impact:** the Bioware .dds header word 4 (`*pUnknown`) is the fread length into a buffer sized ImageGetS3TCSize(w,h,fmt); a larger value (malformed/modded override .dds) overruns the heap and the mip loop walks past the end.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** *pUnknown clamped to [0, ((w+3)/4)*((h+3)/4)*block]. Play-test: load a loose override .dds, check it looks unchanged.

## `ImageDownSampleCompressed` copies from a null source
- **Where:** src/libsource/AURORA/aurimage.cpp `ImageDownSampleCompressed`.
- **Impact:** nLevel outside the mip range leaves pSrc 0; the code allocates and memcpys from null with a length equal to the pointer value. No callers.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns 0. No play-test.

## `listalltextures` overflows its stack name table
- **Where:** src/libsource/AURORA/aurtexman.cpp `listalltextures` (console command).
- **Impact:** `char aNames[1000][50]` with nCount unbounded: more than 1000 listed textures smash the stack.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the loop stops at the array size. Play-test optional (run the console command).

## Never-set arguments to the GL post-process passes (harmless)
- **Where:** src/libsource/AURORA/aurglcore.cpp: FrameBufferModifications/ARB/ATI pass `nUnk`; RenderOverlayConvolution{,ATI,ARB} pass `fUnk`; FinishSoftShadowsARB and RenderOverlayConvolutionARB pass `hDC`/`hGLRC`/`pPBuffer`.
- **Impact:** none; every callee ignores them, the original passes register junk.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** zero/nullptr initialisers. No play-test.

## `List<T>::allocate` / `SetSize` shrink overflow
- **Where:** src/libsource/AURORA/_instances/List.h `List<T>::allocate`; original K2 `List<Vector>::SetSize` 0x74e740.
- **Impact:** SetSize(n) with 0 < n < count (or a direct allocate below the count) copies m_count elements into an n-element buffer: heap overrun on read and write. Latent: no known caller shrinks (the lightning one-shot `SetSize(nSeg + 2)` grows from 2; the lightning update shrinks the counts by hand).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the copy is bounded by min(count, new capacity). No play-test (ASan would catch a shrink).

## `PartEmitter::moveParticleP2PGrav` reads a null Gob's orientation
- **Where:** src/libsource/AURORA/auremitter.cpp, end of moveParticleP2PGrav, the `m_nUnk0x144 & 0x400` arm (K2 0x74ce30..0x74ce48, K1 0x57a940).
- **Impact:** a P2P-gravity emitter with flag 0x400 and no Gob dereferences null (the 0x100 arm just before does test).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the orientation copy is skipped when m_pGob is null; the position copy stays. Play-test optional (needs a Gob-less 0x400 P2P emitter).

## `moveParticleStandard` bounce magnitude read uninitialised
- **Where:** src/libsource/AURORA/auremitter.cpp, the bounce block of moveParticleStandard.
- **Impact:** with `EmitterBounceType` outside {1, 2} `fMagnitude` is junk; unreachable while the global stays 1.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `fMagnitude = 0.0f`. No play-test.

## `MdlNodeEmitterConJoinTwinMysLexia` passes a junk name (harmless)
- **Where:** src/libsource/AURORA/auremitter.cpp (K2 0x6dd4c0).
- **Impact:** none; an unset x1 goes to a ctor that ignores it.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** passes `pv`, as the siblings do. No play-test.

## CSWCLevelUpStats::CalcNumberFeats: the no-class arm indexes class 0xff (src/game/clientcore/nwccreaturestats.cpp)
- **Where:** `CalcNumberFeats`, `nClassPos > 1` (m_nNumClasses 0 or > 2): `pClass = &g_pRules->m_paClasses[0xff]` (K1 and K2).
- **Impact:** reads two bytes from past the classes.2da heap array. Unreached in level-up (a level-up creature always has a class).
- **Fixed under K2_FIX_ENGINE_BUGS:** no class gives 0 normal / 0 bonus feats.

## CSWCMessage::HandleServerToPlayerDoorUpdate: uninitialised generic-door type stored (src/game/clientcore/nwchandlemessage.cpp)
- **Where:** the door update handler. `nGenericType` is read only when nDoorType == 0, and `pDoor->m_Unk0x17d = nGenericType` runs always.
- **Impact:** none visible. Both readers consult m_Unk0x17d only when m_Unk0x17c (the door type) is 0.
- **Fixed under K2_FIX_ENGINE_BUGS:** 0xff (the ctor's value) when not sent.

## CSWCMessage::HandleServerToPlayerPlaceableUpdate: AttachToScene through a null area (src/game/clientcore/nwchandlemessage.cpp)
- **Where:** the placeable add/update. `pArea = GetModule()->m_pArea` is null-tested for the body-bag check, then `pPlaceable->AttachToScene(pArea->m_pScene)`.
- **Impact:** crash if a placeable update arrives with no current area (module transition).
- **Fixed under K2_FIX_ENGINE_BUGS:** the scene attach is skipped without an area.

## CSWCMessage::HandleServerToPlayerItemUpdate_Add / ItemScript_Add: uninitialised model variation on a truncated message (src/game/clientcore/nwchandlemessage.cpp)
- **Where:** both handlers. The appearance byte is read under its own `!MessageReadOverflow(1)`, and CreateItem runs either way.
- **Impact:** an overflowed message creates the item with a junk model variation (wrong or missing model/icon resref). The local server never truncates.
- **Fixed under K2_FIX_ENGINE_BUGS:** variation 0.

## CSWCMessage::HandleServerToPlayerUpdate_PlayerInfo: null creature dereferenced (src/game/clientcore/nwchandlemessage.cpp)
- **Where:** `HandleServerToPlayerUpdate_PlayerInfo`. pCreature comes from GetCreatureByGameObjectID. Most arms test it, but bit 12 (ClearFeats on a null pStats, DeleteSpellLikeAbilityList), 15 (skill ranks), 16 (portrait), 9 (hit points) and 17 (force points) dereference it.
- **Impact:** crash when the server sends player info for an object the client doesn't have (e.g. during an area transition).
- **Fixed under K2_FIX_ENGINE_BUGS:** the message is dropped (return 0, as the bit-13 arm already does) when the creature is missing and the mask carries one of those bits. Gold/XP-only updates still apply.

## CSWSCreatureStats::GetRangedAttackBonus: null main-hand item with an off-hand weapon (src/game/servercore/nwscreaturestats.cpp)
- **Where:** the `a1 == 0` dual-wield arm: `GetRelativeWeaponSize(pItem)` and `GetBaseItem(pItem->m_baseItem)`, with pItem = slot 0x10 possibly null (the top of the function tests it).
- **Impact:** crash for a creature with an off-hand weapon and an empty main hand (the UI prevents it, but a scripted equip or save may not).
- **Fixed under K2_FIX_ENGINE_BUGS:** the dual-wield penalty arm needs a main-hand item.

## CSWSCreatureStats::AddExperience / ReadStatsFromGff: NWN third class slot (src/game/servercore/nwscreaturestats.cpp)
- **Where:** AddExperience's favoured-class pick (K1 `for (i = 1; i < 3; i++)`, K2 `m_classInfo[2].m_level` under `m_numClasses > 2`). ReadStatsFromGff's three `m_numClasses >= 3 && m_classInfo[2]...` checks.
- **Impact:** none. The class count is clamped to 2 at load (ReadStatsFromGff) and never grows past 2, so the slot-2 reads (which land on the ability-score bytes) are dead.
- **Fixed under K2_FIX_ENGINE_BUGS:** the slot-2 code compiles out.

## CSWSCreatureStats::SetExperience: tag compared by address (src/game/servercore/nwscreaturestats.cpp)
- **Where:** `m_pOwner->m_sTag.CStr() != ""`, a pointer comparison with the literal (the original does `cmp x0, x8`).
- **Impact:** none. It only picks the tag for a "Cannot reduce XP" message that is formatted and then dropped.
- **Fixed under K2_FIX_ENGINE_BUGS:** `m_sTag != ""`.

## CSWGuiMainCharGen::AbilitiesChanged: hit die of an unassigned class pointer (src/game/clientcore/swguimainchargen.cpp)
- **Where:** `pClass` is assigned only inside the class loop and read after it for `m_nHitDie`.
- **Impact:** none in practice (chargen always has one class). With no class it reads a stack pointer.
- **Fixed under K2_FIX_ENGINE_BUGS:** no class adds 0.

## CSWGuiQuickPanel::HandleInputEvent: junk control pointer (src/game/clientcore/swguiquickpanel.cpp)
- **Where:** key 0x29, `OnCancelPressed(pControl)` with pControl uninitialised (the original passes the stale x1).
- **Impact:** none. OnCancelPressed ignores the argument.
- **Fixed under K2_FIX_ENGINE_BUGS:** nullptr.

## CSWMiniGame::Update (K1): player flags read through a null player (src/game/clientcore/swminigame.cpp)
- **Where:** the K1 arm `if ((m_pPlayer->m_flags & 1) != 0) DoHitCheck();`. K2 tests m_pPlayer first.
- **Impact:** crash on the frame after the player's swoop/turret dies if the minigame is still updating (CSWTrackFollower's death path calls SetPlayer(0)).
- **Fixed under K2_FIX_ENGINE_BUGS:** the K2 null test. Play-test: die in a K1 swoop race / turret fight on the host build.

## CSWMiniGame::Update (K2): removed objects destroyed through the base dtor -- fixed later (see "K2 `CSWMiniGame::Update` -- a dead enemy is freed without its destructor" below)
- **Where:** K2 0x3782bc: `delete pObject` on a CSWMiniGameObject* whose K2 dtor is not virtual (K1's is), for every object with m_bUnk10 set (CSWTrackFollower's death path flags dead enemies).
- **Impact:** a dead enemy's CSWTrackFollower part is never destroyed: its models (those with no "die" animation were already deleted), m_pTrack, gun banks and m_pSounds[0] (Stop + delete) leak, so the engine sound may keep playing after death. The memory is lost per kill.
- Proposed fix: delete followers through `CSWTrackFollower*` (it has a virtual dtor). But ~CSWTrackFollower also does `g_adjustWake = 0` (K2), which would disable the player's MG_Distort wake effect on the first enemy kill. A correct fix must keep that latch: run the derived teardown without the latch, or re-set it. Needs a K2 swoop / turret play-test either way.

## `CSWGlobalVariableTable::ReadTableWithCatalogue`: a full table stores through slot -1; CatString reads past five strings
- **Where:** K2 0x68367c / K1 0x3dbe54. When AddIdentifier* fails (that type's table is full) it logs "won't fit" into a discarded string and then stores through `m_aIdentifiers[-1].m_nIndex` (K2 0x683e34 -> 0x683d44). The CatString walk indexes the 5-element `aStrings` with the catalogue index, which is not bounded.
- **Impact:** a save with more globals than fit, or more than five string catalogue entries, writes one value into a slot picked by junk bytes before the table / copies a CExoString from past the stack array. Only crafted or modded saves reach it.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a name that won't fit is skipped; catalogue strings past the fifth are skipped. No play-test needed.

## `CSWGlobalVariableTable::ReadTableWithoutCatalogue`: the string walk starts past the strings it read
- **Where:** K2 0x6831ec / K1 0x3db364. `nString` is the count of pre-read ValString entries (up to 5) and then the identifier walk keeps incrementing it for each string identifier, reading `aStrings[nString++]`.
- **Impact:** string globals take the wrong value (aStrings[1] for the first after one was read), and from the sixth on they copy a CExoString from past the stack array (crash on a junk pointer).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the walk counts from 0 and stops at 5. It only matters for a catalogue-less GVT. No play-test needed.

## `CSWSDoor::EventHandler`: the melee arm reads the weapon of a null attacker
- **Where:** K2 0x536cfc / K1 0x46d8d8, EVENT_ON_MELEE_ATTACKED: `pAttacker->m_pInventory->GetItemInSlot(...)` with pAttacker from GetCreatureByGameObjectID, which is only null-checked around it.
- **Impact:** a melee event whose caller is not a creature (or is gone) crashes.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the weapon lookup runs only for a creature attacker. (The trap arm's `pTripper`, already listed at engine-bugs.md "CSWSDoor::EventHandler (0x537c9c)", is now fixed under the flag too: the faction compare is skipped for a null tripper.)

## `CSWSEncounter::ReadEncounterFromGff`: the activation-area list is read from element 3, into an array sized by another field
- **Where:** K2 0x59ce20 (`mov w3,#3` at 0x59d88c) / K1 0x488794 (0x4892a4). Every slot i reads `AreaList[3]`, and the array is `new[AreaListMaxSize]` filled to `AreaList` count.
- **Impact:** after a reload every encounter's "creatures that entered" list is element 3 repeated (or uninitialised slots when the list has fewer than four entries), so the de-duplication in AddToActivateAreaList / spawn conditions read the wrong ids; a save whose count exceeds its saved max size overflows the heap.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** element i is read and the capacity is raised to the count. Play-test suggested: save and reload inside an encounter trigger, check it still spawns once.

## K1 `CSWSEncounter::PadOutCreatureCount`: fills past the spawn array when RecCreatures > MaxCreatures
- **Where:** K1 0x487dd4. It tops the list up to `RecCreatures - active` with no bound; the list is `new CEncounterListEntry[m_nMaxCreatures]` in BuildSpawnList.
- **Impact:** an encounter blueprint whose RecCreatures exceeds MaxCreatures overflows the heap array.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** padding stops at m_nMaxCreatures. No play-test needed.

## `CServerAIMaster::UpdateState`: an event for a null object dereferences it
- **Where:** K2 0x67c700 (0x67ca24 loads the type before the `cbz x0`) / K1 0x53ae98. Only the first dispatch arm tests `pEventObject != 0`.
- **Impact:** latent: GetGameObject's SUCCESS implies a stored object.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a null object's event data is deleted.

## K1 `CExoBaseInternal::GetDirectoryList`: the time-sorted listing (b == 2) is broken in both arms
- **Where:** K1 0x6456c8. OBB arm: inserts at the index left over from an earlier entry; disk arm: inserts only when `stat()` fails (`cbz w22` after stat, 0x6463d4), with an unset `st`.
- **Impact:** none observed: no caller passes b == 2.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both arms sort by time, the disk arm on stat success.

## `CExoBaseInternal::SpawnExternalApplication`: the process and thread handles leak
- **Where:** K2 0x7a7c80 / K1 0x648128. Latent: every port's CreateProcessA fails.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both handles are closed after a successful launch.

## `CTlkTable::AddFile`: index 7 is accepted for a 7-row file grid
- **Where:** K2 0x7c8108 (`cmp w2,#7; b.hi`) / K1 0x667aa0.
- **Impact:** latent (the LIVE loop passes 1..6); index 7 would overwrite m_gender and the next member.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** indices >= 7 are rejected.

## `CExoSoundSourceInternal::SetPosition`: a 2D-only source dereferences the null 3D voice
- **Where:** exosoundsourceinternal.cpp SetPosition (K1 ends in the tail call at 0x681350). The channel test falls back to the 2D voice, but the call passes `m_p3DVoice->m_nChannel`.
- **Impact:** positioning a playing source that has only a 2D voice crashes.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns when there is no 3D voice. A sound play-test is worth doing.

## K1 `CSWSPlayer::PackCreatureIntoMessage`: scalar delete of a new[] buffer
- Harmless (unsigned char). **Fixed under `K2_FIX_ENGINE_BUGS`** (delete[]).

## AurResGetData / getnextline_res: unbounded copies into the resource read buffer (src/libsource/AURORA/aurconsole.cpp)
- **Where:** `AurResGetData`, `getnextline_res` (K1 and K2).
- **Impact:** a memory-blob resource's read buffer is 8 KB (`OpenResource(0x2000)`), and file resources get 500000 bytes. `AurResGetData(nBytes)` copies or freads nBytes into it unchecked; aurtexman's packed-blob `Parse(List<Vector>)` asks for nCount*12, so a packed list of more than 682 vectors in a memory resource overruns the heap. `getnextline_res` copies one line up to the newline with no bound (a line over 8191 bytes overruns). K1's CR LF test reads one byte past the blob.
- **Fixed under K2_FIX_ENGINE_BUGS:** `AurResGetData` reallocates the buffer to nBytes+1 when it is too small. Line copies stop at m_nSize-1 (a longer line is split). The LF peek checks `i < m_nLength`. Play-test: load a module and a GUI; texture txi/font parsing goes through here.

## CAurGUIImageInternal::DrawBuffered: rotation of 315..360 degrees writes through an uninitialised index (src/libsource/AURORA/aurgui.cpp)
- **Where:** `DrawBuffered`, the `(int)((fAngle + 45) / 90)` switch (K1 0x72f5dc, K2 0x59af7c: `cmp w12,#3; b.hi`).
- **Impact:** quarter index 4 (fAngle in [315,360)) or any index outside 0..3 leaves `anUV[4]` as stack junk. Each vertex's uv pair is then stored at `cm_fGUIBuffer[anUV[k]*6 + nUsed + 4]`, an out-of-bounds write of up to ~1.5k floats past the vertex, and the uv mapping is wrong.
- **Fixed under K2_FIX_ENGINE_BUGS:** the index wraps `& 3` (315-360 degrees draw as 0). Play-test: GUI elements with a rotation near -45 degrees or 315 degrees (rare).

## MdlNodeTriMesh / MdlNodeDanglyMesh::InternalPostProcess: null first vertex on a > 32767-vertex mesh (src/libsource/AURORA/aurpostprocess.cpp)
- **Where:** both functions: `pFirst = (short)m_uGate0x1c0 >= 1 ? m_lUnk0x80.m_pData[0] : 0`, then `pFirst->m_nFlags0x54`.
- **Impact:** the vertex count is stored as an unsigned short and tested as a short, so a mesh with more than 32767 vertices (or none) dereferences null at model load.
- **Fixed under K2_FIX_ENGINE_BUGS:** post-processing of such a mesh stops (the mesh is left without vertex arrays rather than crashing). No shipped mesh is known to hit it.

## CSWGuiIndicator::DrawJointedLine: anchors 4-63 draw with uninitialised geometry (src/libsource/SWGui/swguidrawable.cpp)
- **Where:** `DrawJointedLine`, the switch on the 6-bit `m_nStartAnchor` (K2 0x6391c4 `cmp w12,#3; b.hi`, K1 the same).
- **Impact:** a GUI indicator whose start anchor is outside 0..3 draws its edge tiles, corner and caps with 15 uninitialised coordinates and rotations (garbage quads on screen). It is data-driven (the GUI resource's anchor).
- **Fixed under K2_FIX_ENGINE_BUGS:** such an indicator draws nothing.

## CVirtualMachineStack::AssignLocationToLocation: self-copy is a use-after-free (src/libsource/nwscript/nwvmachineinternal.cpp)
- **Where:** `AssignLocationToLocation(nSrc, nDst)` with nSrc == nDst.
- **Impact:** a string cell is deleted and then copied from (`*pSrc` on freed memory). An engine-structure cell is Destroyed and then Copied. nwnsscomp output never copies a cell onto itself, so it is latent.
- **Fixed under K2_FIX_ENGINE_BUGS:** a self-copy is a no-op.

## CVirtualMachineStack::LoadStack: StackPointer beyond TotalSize overflows the heap (src/libsource/nwscript/nwvmachineinternal.cpp)
- **Where:** `LoadStack`, from the saved script situations of a savegame.
- **Impact:** the cell arrays are `new[TotalSize]` and the read loop runs to `StackPointer`, so a corrupt or hand-edited save with StackPointer > TotalSize writes past both arrays.
- **Fixed under K2_FIX_ENGINE_BUGS:** StackPointer is clamped to TotalSize. A valid save is unaffected. Play-test: load a save that has pending DelayCommand / action scripts.

## CVirtualMachineInternal::ReadScriptFile: null script name dereferenced (src/libsource/nwscript/nwvmachineinternal.cpp)
- **Where:** `ReadScriptFile(CExoString* pScriptName)`. It tests pScriptName for the resref and then does `m_sName = *pScriptName`.
- **Impact:** a null name crashes after the resref lookup. No caller is known to pass null.
- **Fixed under K2_FIX_ENGINE_BUGS:** the situation gets an empty name.

## `MaxTree::foobar` / K1 `MaxTree::operator delete` — a negative-flag tree that is not a Model reads `[0 + 0xe0]`

- **Where:** `MaxTree::foobar` (K2) / `MaxTree::operator delete` @0x5afe4c (K1). After the destructor call the
  model view is taken as `(m_cUnk0x68 & 0x7f) == 2 ? this : 0` (`csel x8, x19, xzr, eq`) and `m_pUnk0xe0` is read
  through it unconditionally.
- **Impact:** a null read for any tree with the high flag bit whose kind is not 2. Not seen in play (only Models
  carry the bit), so latent.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** null-checked. No play-test needed.

## `Gob::AttachToPart` — an attach type above 3 installs the type number as the behaviour pointer

- **Where:** `Gob::AttachToPart(CAurObject*, CAurPart*, int)` @0x5bde00 (K1). The switch has arms 0-3 only;
  `cmp w20,#3; b.hi` skips to `SetBehavior(x20)` with x20 still holding nType.
- **Impact:** `SetBehavior((CAurBehavior*)nType)` then `SetScene`: a wild pointer. The one caller (swmgguns) passes 0.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** pBehavior starts at 0 (detach). No play-test needed.

## `PartTriMesh::HitCheckGeom` — null-tests the node, then dereferences it

- **Where:** `PartTriMesh::HitCheckGeom` @0x6e4828: `cbz x25` skips only the 0x821 flag test; the next instruction
  is `ldrh w8, [x25, #0x1c0]`.
- **Impact:** null read for a vertex primitive without a node (latent).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns 0 (no hit) for a null node.

## `CSWSMessage::SendServerToPlayerGameObjUpdateVisEffect` — a beam with a dead source writes through null

- **Where:** @0x5f59fc (K2). `pSource` is `GetGameObject(sourceId)->AsSWSObject()` or 0; the beam arm
  writes `pSource->m_position.{x,y,z}` with no test (the function's own header comment already noted it).
- **Impact:** a server-side null read when a beam effect is re-sent after its source object is gone.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** writes vPos instead (the message keeps its layout). Low-risk; a beam
  VFX (Force Lightning / drain) on a creature that then dies or despawns would exercise it.

## `CSWSMessage::SortObjectsForGameObjectUpdate` — `pObject != 0` tested, then `pObject->AsSWSCreature()` in the else arm

- **Where:** @0x5e8824: `cbz x2` falls into `ldr x8,[x21]; blr` with x21 = pObject.
- **Impact:** a null viewer object faults instead of producing an empty list (latent; callers pass the player).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `else if (pObject != 0)`.

## `CSWSMessage::TestPartyObjectUpdateDifferences` — the viewer creature is null-tested once and then dereferenced

- **Where:** fnindex K2 ~7012. `nReputation` honours `pViewerCreature != 0`; the area comparison right after reads
  `pViewerCreature->m_cachedAreaId`.
- **Impact:** a player whose game object is not a creature faults (latent).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the area/position test is skipped without a viewer.

## `CSWSMessage::WriteGameObjUpdate_MajorGUIPanels` — an open container that is neither item nor placeable

- **Where:** fnindex K2 8224 (both the K1 and K2 arms). `pRepository` is set only for OBJECT_TYPE_ITEM /
  OBJECT_TYPE_PLACEABLE and then passed to `WriteRepositoryUpdate`, whose first statement is
  `pRepository->ItemListGetCount()`.
- **Impact:** an indeterminate pointer dereference if the container id resolves to any other object type
  (e.g. a creature's inventory opened through a script, a stale id reused by another object).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** starts at 0 and the call is skipped.

## `CSWSCreature::CanUseItem` — the proficiency test reads a null base item

- **Where:** fnindex 1063 (both games). `GetBaseItem` returns 0 for an index < 0 or >= the table size; the
  first test tolerates it (`pBaseItem == 0 || ...`), the `m_Unk0x04 & 0xfffff` (K1 0x3ffff) test does not.
- **Impact:** a crash equipping/inspecting an item whose base item is outside baseitems.2da (mods, damaged saves).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns the usability verdict without the proficiency test.

## `CSWSCreature::CheckItemAttributeRestrictions` (K2) — an unknown ability subtype leaves the score unset

- **Where:** @0x5432fc: `cmp w27,#5; b.hi #0x543370` — clang folded the indeterminate compare, so the orig skips
  the restriction for a subtype > 5.
- **Impact:** UB in the source; in the shipped binary the restriction is silently ignored.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `default: continue;` (the shipped behaviour, made defined).
- **Also noticed (not a warning row, NOT fixed):** `case 3` reads `m_nCONBase`, so an INT requirement (ability 3)
  is checked against Constitution; the jump table confirms only five distinct targets for six cases. Worth a
  gated fix and a play-test with an INT-restricted K2 item.

## Debug console `TestLocationIsSafe` / `LocateNearestPathPoint` / `ValidateCreaturePath` — `%d` into a 64-bit id

- **Where:** nwscreatureai.cpp's console handlers.
- **Impact:** sscanf writes 4 of the 8 bytes; correct only on little-endian with the high half pre-zeroed.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `%llu`.

## `CSWSCreature::SavingThrowRoll` — the queued saving-throw message goes through a null source

- **Where:** fnindex K2 3654. `pSource` is the source object's creature view or 0; with bQueueToAttack the message is
  added to `pSource->m_pCombatRound`'s current attack unconditionally.
- **Impact:** a crash when a queued save is rolled against a non-creature (placeable trap, AoE) or a source that no
  longer exists.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** falls back to the broadcast-and-delete arm.

## K2 grenade path in the action/combat code — the already-documented null `pSpell` re-dereference

- **Where:** nwscreatureaicombat.cpp (~3952, the `ConvertGrenadeSpellID` arm). The comment already described it
  ("Kept as in the source").
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `&& pSpell != 0`.

## `CScriptCompilerInternal::CompileFile` — the parse-error path releases the wrong include slot

- **Where:** @0x7ea618 (K2): success path `ldr x0,[x8,#0x250]` (slot depth-1), error path `ldr x0,[x8,#0x298]`
  (slot depth) after `m_nIncludeFileDepth++`.
- **Impact:** a nested-include compile error releases the NEXT level's resource (never demanded, or stale) and at
  the 16th level indexes one past `m_includeFiles[16]`. The error path also never decrements the depth (left as is).
  Script compiler only.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** releases slot depth-1.

## `CScriptCompilerInternal` symbolic listing — `CONSTS %04x` gets a 64-bit length

- **Impact:** a format mismatch (benign on AArch64). **Fixed under `K2_FIX_ENGINE_BUGS`:** cast to unsigned int.

## `ParseNode` / `MdlNode::InternalParseField("parent")` — empty tokens leave the 64-byte name buffers indeterminate

- **Impact:** ascii-model import only: a node/parent line without its tokens compares and copies stack garbage.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the buffers start empty.

## `CSWSItem::RemoveUpgrade` — the copy is sized by the upgrade item's property count, filtered by the slot tag

- **Where:** nwsitem.cpp. `new CSWItemProperty[m_nPassiveProperties - pUpgrade->m_nPassiveProperties]`, then every
  property whose `m_Unk0x12 != nSlot + 1` is copied in.
- **Impact:** if the number of tagged properties is not exactly the upgrade item's current count (an upgrade whose
  own properties changed, a save edited by a tool, two upgrades tagged into one slot), it overruns the heap block or
  leaves trailing properties indeterminate.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** sized by m_nPassiveProperties, count = what was kept. Play-test: K2 workbench
  remove/replace an upgrade, check the item's properties.

## `CSWSItem::CompareItem` — the property loops index past the other item's properties

- **Where:** fnindex K2 3722. Each of the three loops (active, passive, passive again) takes the other item's i-th
  property as 0 when i >= its count and then dereferences it.
- **Impact:** a crash comparing (stack-merging / repository matching) two otherwise-equal items when this one has
  more properties.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a missing property means "different".

## `CSWSItem::DecrementUses` — an out-of-range property index switches on null

- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns early.

## `spawnbinarytile` (K2 console) — no "mainscene" means an unbounded scan

- **Where:** aurscene.cpp. The fallback `for (j = 0; ; j++) _stricmp("scene", Scenes.m_pData[j]->m_szName)` has no
  exit and walks off the scene list.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** returns "no mainscene".

## `IosLocalizedStringsImpl` (K2 FILE* loader) — unterminated buffer, unchecked malloc, out-of-bounds clamp

- **Where:** src/port/ios/IosLocalizedStrings.cpp. `malloc(nSize)` unchecked, never NUL-terminated before `strtok`,
  and `pszLine[0x1ff] = 0` on every token writes 511 bytes past a short line's start (past the block for lines near
  the end of the file).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `malloc(nSize + 1)` checked and terminated at the bytes read; the clamp only
  for lines longer than 0x1ff. Play-test: any screen that shows the localized-strings table (K2 desktop/mobile
  title/options strings).
- **Final pass (2026-10-22):** a failed `ftell` (nSize -1, then `malloc(0)` and `fread(..., (size_t)-1)`) now counts as an empty file. No play-test (ftell does not fail on a regular file).

## `CClientOptions::SetCreatureWindMode` — the player's wind goes to the last object the loop saw

- **Where:** clientoptions.cpp. The comment above the function says the player is skipped in the loop because it is
  handled afterwards; the afterwards block tests `pPlayerCreature->m_fadeState` and then calls `pObject->SetWind`,
  pObject being the loop's last visited object (indeterminate for an empty area, possibly null).
- **Impact:** the controlled character never gets the creature wind, some other object gets it twice; a crash for an
  empty area.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the block uses pPlayerCreature. **Play-test:** toggle creature wind
  (options / `setcreaturewindmode`) and watch the PC's robe/cloak.

## `CSWParty::Update` — the retreat loop dereferences a possibly-null leader

- **Fixed under `K2_FIX_ENGINE_BUGS`:** the loop runs only with a leader.

## Capture-file replay (`clientinput.cpp`) — an extra sscanf argument and `%d` into an unsigned size

- **Fixed under `K2_FIX_ENGINE_BUGS`:** the K1 header call; `%u` for the size. Benign in practice.

## `CExoRandInternal` -- the K1 seed is an uninitialised heap block (and `GenSeed` copies stack junk)
- **Where:** the constructor, K1 @0x64f044 / K2 @0x7a38c4. The 0x80-byte entropy ring is `new unsigned char[0x80]`
  and `GetString(0x10)` XOR-folds it into the four `ReSeed` words at once. K2 first `Add`s the performance counter
  and `GetSystemInfo` (0x38 bytes); K1 adds nothing, so its whole seed is whatever the allocator hands back (for a
  zeroed block every NUL becomes `Rand() % 250 + 1` of the fixed initial state: the same seed each launch). `Rand()`
  never reads the ring again (`SignalEvent` only feeds it, for `GetString`), so the constructor's seed fixes the
  whole stream. `GenSeed` (K1 @0x64f238, K2 @0x7a3ae8, cl.exe C4700) copies an uninitialised 128-byte stack buffer
  into the ring; it has no caller in either image (no branch to it, no PLT slot), so the ledger's earlier "K1 seeds
  from GenSeed" was wrong. The destructor frees the `new[]` ring with scalar `delete` (K1 @0x64f674, K2 @0x7a3f48).
- **Impact:** in K1 the random stream depends on the heap block's leftover bytes and can repeat from launch to launch.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the ring is filled before the first fold by `k2host_entropy`
  (platforms/shim/shim_time.cpp: `rand_s` on Windows, `/dev/urandom` elsewhere, with the performance counter, the
  wall clock and a stack address mixed over it); K2 then adds its timers as before. `GenSeed` fills the ring the
  same way; the destructor uses `delete[]`. **Play-test (optional, K1):** load the same save twice from a fresh
  launch and compare the first attack rolls in the combat log: they should differ.

## K2 `CSWMiniGame::Update` -- a dead enemy is freed without its destructor
- **Where:** K2 0x3782bc (`bl ~CSWMiniGameObject` then `operator delete`, cl.exe C5205): every object with
  `m_bUnk10` set is deleted through `CSWMiniGameObject*`, whose K2 destructor is not virtual (K1's is). Only
  `CSWTrackFollower::OnDeath` sets the flag (0x4487dc), and only for an enemy (the player goes to `SetPlayer(0)`)
  whose models all lacked a "die" animation, so those models were already detached and deleted. Every other
  teardown (an enemy that plays "die" -> `TrackFollowerPartDie`, the player, `ClearEnemies`, `~CSWMiniGame`)
  goes through the virtual destructor; obstacles are never flagged.
- **Impact:** `~CSWTrackFollower` never runs for such an enemy: its track Gob (still in the scene), its engine
  sound (`m_pSounds[0]`, already stopped) and its memory leak; the vehicle model itself is already gone.
- **The wake latch:** `~CSWTrackFollower` also clears the global `g_adjustWake` that the follower constructor sets
  (the MG_Distort wake colouring in auremitter.cpp). In the original any follower's real teardown clears it for
  every follower: the first enemy that dies with a "die" animation, or the player's death, turns the wake colouring
  off until a new follower is built, and after `RestartMiniGame` (the new game is built before the old one is
  deleted) it ends off.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `Update` deletes a flagged follower through `CSWTrackFollower*`, with
  `m_flags` bit 2 set first so a gun bank still playing a gun's death animation is left to free itself (`GunDie`),
  as on every other death path (the script `OnDeath(1)` path leaves the bit clear, and the destructor would free a
  bank a pending "donedie" callback still names). `~CSWTrackFollower` no longer clears `g_adjustWake`; the last
  `CSWMiniGame` destroyed clears it (a live count, so a restart keeps it on), and a non-minigame area never draws
  with it set. The follower destructor also clears `m_pTrackFollower` in each bank it leaves alive, so a late
  `GunDie` no longer calls `RemoveGunBank` through the freed follower (the original's use-after-free on every death
  path with die-animated guns). **Visible change:** the MG_Distort wake colouring stays on for the whole minigame
  (the player's and the remaining enemies' after a kill, and after the player dies) and goes off when it ends.
- **Play-test (K2, host build):** an Ebon Hawk turret fight (any scripted space attack) and a swoop race (the Nar
  Shaddaa or Onderon track). (1) Destroy enemies: each explodes and disappears with no leftover track model, no
  engine sound that keeps playing, no crash, including late in a long fight. (2) After the first kill, the
  player's and the other enemies' wake / exhaust distortion keeps drawing. (3) Fail or restart a race: the wake
  still draws. (4) Leave the minigame: the next area renders without distortion artefacts.

## K2 `CSWSCreature::CheckItemAttributeRestrictions` -- an INT requirement is checked against CON
- **Where:** K2 @0x543320 (K1 has no such function). The ability switch's jump table (0x9c9f3e: `>A;;`) sends case 2
  and case 3 to the same arm (0x543450, `+0x13d` = `m_nCONBase`); `m_nINTBase` (+0x13f) is never read.
- **Impact:** an item whose ability-requirement property (0x45) names Intelligence is usable by a creature with
  enough Constitution, whatever its Intelligence.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** case 3 reads `m_nINTBase`. **Play-test:** only if an item with an INT
  requirement exists (none known in the stock data; mods): equip it with high CON and low INT, and the reverse.

## `CSWSObject::LoadActionQueue` -- a save's NumParams indexes the 13-entry stack arrays unchecked
- **Where:** K2 @0x642870 / K1 @0x4be624: `NumParams` (a GFF WORD) bounds both loops over `apValues[13]`,
  `anTypes[13]`, `anAlloc[13]`; the original has no compare against 13.
- **Impact:** a save (or mod) with more than 13 parameters on a queued action overwrites the stack.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** clamped to 13 (`AddAction` takes 13). No play-test (stock saves never exceed it).

## `Gob::ReplaceTexture` / `Gob::ReplaceTextureSubtree` -- `n` sizes copied into `int paSizes[0x100]` unchecked
- **Where:** K2 @0x6e659c / @0x6e67f4 (and K1): the count comes from the caller and is also passed on to
  `Material::ReplaceTexture`, which reads that many.
- **Impact:** a count above 256 overruns the stack array. The callers pass palette-layer counts (at most 10).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `n` is clamped to 0x100. No play-test.

## `new[]` buffers released with scalar `delete` (the rest of the class)
- **Where:** `AurBuild2DMipMaps` (K2 0x713c44 / 0x713cac, its mip copies), `CSWCObject::InsertFireForgetAnimation`'s
  grow (K2 0x4ace44, K1 0x2bd794), the `CExoRandInternal` ring (above), `CServerExoAppInternal::m_pPendingAuthIds`
  (above), `C2DA::Unload2DArray`'s binary offset tables, `CERFRes::Reset`'s payload, `CGameEffect::m_pIntegers`
  (destructor and `SetNumIntegers`), `~CSWRumbleMotorData`, `CAppManager`'s large temp buffer, and K1
  `CExoString`'s six buffer frees (K2 uses `delete[]`).
- **Impact:** none observed: every element type is trivially destructible and bionic / libc++ / the UCRT free both
  forms alike. ASan's alloc-dealloc-mismatch check (on by default on Linux) reports them.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `delete[]`. No play-test.

## `CSWSMessage::ComputeVisibilityLists` -- the snapshot is searched at the outer index
- **Where:** K2 @0x5dcf68: the inner loop runs `nCount` times comparing `pLive->m_objectId` with the snapshot entry
  at the OUTER index `i` (`ldr x15,[x11,x15]` with `x15 = i << 3`, outside the inner loop; `csel x12,x15,x12,eq`
  keeps it as the match). K1 @0x4dcbe8 is the same program with the dead inner loop folded away (a straight
  position-by-position compare).
- **Impact:** a creature's visibility list counts as changed whenever two entries swap places, so the server
  resends a visibility update the client already has. Never a missed update (a real change at position i still
  differs at i). Network traffic only; single-player sees nothing.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the snapshot is searched by `j` (the last entry with the id is the match, as
  the original's loop keeps the last). No play-test (no visible difference by construction).

## `CERFKey::SetName` / `CERFFile::AddResource(CExoString&)` -- a long name overruns the 16-byte key name
- **Where:** `CERFKey::SetName` K2 @0x7b981c / K1 @0x66b104: `memcpy(m_sName, CStr(), strlen)` with no cap (K2
  `bl strlen; sxtw x2, w0; b memcpy`). `CERFFile::AddResource(CExoString&)` K2 @0x7b8458: `__memcpy_chk(name,
  stem, strlen, 0x18)` -- the fortify bound is the whole 0x18-byte key, not the 16-byte name.
- **Impact:** a file stem of 17..24 characters writes into `m_nID` / `m_nType` (rewritten next) and `m_nUnk0x16`
  (written to the archive as junk); a longer one overruns the heap key in SetName and aborts (fortify) in
  AddResource. ERF names are resrefs (at most 16), so only a long file name added to a save / mod archive reaches it.
  The `CRIMRes::SetName` sibling was clamped in batch 1.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** clamped to 16 bytes; AddResource names the key through SetName. No
  play-test (stock data never has a stem over 16 characters).

## `FModAudioSystem::PauseChannel` / `ResumeChannel` (K2) -- the channel lookup has no end() or key test
- **Where:** K2 @0x346fa4 / @0x347060: an inlined `lower_bound(nChannel)` walk, then `ldr x8,[x8,#0x28]` (the
  node's value) with neither an `end()` compare nor a key-equality compare; every other channel entry point uses
  `GetChannelInfoByUID` (`find`, null on a miss). K1 has no such pair.
- **Impact:** an id that is not in the map pauses / resumes the NEXT channel (a stale id after the map was rebuilt),
  or, above the largest id, writes `m_bPaused` into and calls `setPaused` through the map's header node (a crash).
  Callers pass ids the system handed out, so only a stale handle reaches it.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `GetChannelInfoByUID`, return 0 on a miss. **Play-test (K2):** pause the game
  with music and ambient sound playing (and in a conversation), resume: sound pauses and resumes as before.

## K2 `CSWSEncounter::BuildInitialSpawnList` / `ReadEncounterFromGff` -- a RecCreatures above MaxCreatures (or a negative NumberSpawned) spawns past the list
- **Where:** K2 @0x59c644: the spawn count is `rand() % (MaxCreatures + 1 - RecCreatures) + RecCreatures` (0x59c684 `ldr w9,[x21,#0x360]; sub; add #1; sdiv w10,w0,w8; msub; add w26,w8,w9`), and one CEncounterListEntry per spawn is written into BuildSpawnList's `new CEncounterListEntry[m_nMaxCreatures]` with no other bound (K1 @0x48760c is a different, bounded body; its K1 arm is the phase-4 entry above). `ReadEncounterFromGff` (K2 0x59ce20, K1 0x488794) loads a save's NumberSpawned unchecked; a negative value widens K1's `nCount >= MaxCreatures - ActiveCount` bound past the array.
- **Impact:** with Rec > Max the modulus is negative and the count can pass Max; with Rec == Max + 1 it is `% 0` (arm64 `sdiv` gives 0, so rand() + Rec; SIGFPE on x86). Either way a heap overrun.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** K2 draws `rand() % range + Rec` only when range > 0 and otherwise spawns MaxCreatures (rand() is still called once, so the RNG stream is unchanged for good data); a negative NumberSpawned loads as 0. No play-test with stock data (Rec <= Max); a modded .ute with RecCreatures > MaxCreatures now spawns MaxCreatures.

## `ExecuteCommandBeginConversation` -- the speaker pick reads a null player
- **Where:** K2 @0x567eec / K1 @0x510da0: pPlayer (the local player's client object) is null-tested (`cbz x22`) only around the facing snapshots; the speaker pick reads `ldr x1,[x22,#0x58]` untested (K2 0x5681f8, K1 0x5110a8), and so does the m_oidSelf test.
- **Impact:** a conversation started by an object-event script before the PC exists dereferences null.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `pPlayer != 0 &&` on both comparisons. No play-test (not reachable in practice).

## `CSWCAreaOfEffectObject::LoadEffect` -- a negative NumActxx overruns the instance arrays
- **Where:** K2 @0x3fc0a0 (sum at 0x3fc6f4..0x3fc720), K1 @0x25d5ec (0x25dc40..0x25dc6c): the three band counts are added (`ldrsw [x20,#0x16c/#0x170/#0x174]`, `add`, `cmp w26,#1; b.lt`) and that many instances allocated, but each band's loop runs to its own count.
- **Impact:** with bands {5, -3, 0} the total is 2 and band 0 writes five entries into two-entry arrays (heap overrun). Only a modded vfx_persistent.2da has a negative NumAct.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a negative band reads as 0. No play-test (stock data).

## K2 `CSWGuiMainInterface::OnPersonalActionExit` -- a target-action button indexes past the 7-entry personal arrays
- **Where:** K2 @0x421f48. `CSWGuiMainInterfaceAction::Initialize` (@0x4217a8) registers it as event 1 on every action widget's button, the three target-action widgets included (0x421ab4..0x421acc, unconditional), whose user data is 6..8. The handler indexes `[x19 + w20*4 + 0x25e8]` (m_personalActionIds[7]) and `[x19 + w20*16 + 0xc8]` (m_personalActions[7]) with no bound.
- **Impact:** for 7 (and 8) it reads m_Unk0x2604 as the action id and m_targetActionMenu's vptr and next word as a CExoArrayList, then walks it (gated on the m_Unk0x1d108 latch the personal arrows and OnDefaultActionLeft set): a bogus action-bar revert or a crash. (Target widget 0's user data 6 is also personal slot 6; not fixed.)
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the lookup runs only for `0 <= nAction < 7`; the latch clear and the panel bookkeeping still run. **Play-test (K2):** press a personal-action arrow (or tap an action icon with the action menu open), then move the pointer off a target-action button with a target selected: nothing happens (no flash-revert of the action bar, no crash).

## `CSWGuiInGameCharacter::RotateCharacterLeft` / `RotateCharacterRight` -- an `(int)` member registered as a control handler
- **Where:** K2 @0x463460 / @0x4635d0, K1 @0x32ad98 / @0x32af08 (`...RotateCharacterLeftEi`, address taken through the GOT for AddEvent, reloc K2 @0xc61290, K1 @0x7cec98). AddEvent calls it as `void (CSWGuiObject::*)(CSWGuiControl*)`, so the int receives the low half of the control pointer.
- **Impact:** none: the bodies ignore the argument. Undefined behaviour only.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `OnRotateLeft` / `OnRotateRight(CSWGuiControl*)` (host only) call them with 0 and are registered with an ordinary derived-to-base `static_cast`. No play-test needed (rotation is identical); the character screen's rotate buttons are the place to look if anything changed.

## `CResSET::CreateSectionTable` -- an LF-only file takes the "no terminator" arm for every line
- **Where:** K2 @0x3ef618, K1 @0x2eefc4: `cmp x0, x21; cset w25, hs` with x0 = strchr('\r'). `bNL = pNL && pCR >= pNL` is false when the file has no '\r', so the line end is 0 and `__strncpy_chk(buf, p, size - idx, 256)` copies the rest of the file (also the v2102 note-read item above).
- **Impact:** a fortify abort once more than 256 bytes remain, and `buf[size - idx] = 0` writes past the stack buffer. Stock .set files are CRLF.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `bNL = pNL && (!pCR || pCR >= pNL)`, the nearer terminator. No play-test (only an LF-saved mod .set reaches it).

## `PartSetCanDownSample` -- the source's `&` null test does not guard the load
- **Where:** K2 @0x6e6a04, K1 @0x5bd520. The source `(pNode != 0) & ((pNode->m_nFlags & 0x21) == 0x21)` evaluates `m_nFlags` even for a null node; the original branches first (K1 `cbz x1` at 0x5bd540, then `ldrh w8,[x1]`), and only `&` reproduces its code, so the original source had it too.
- **Impact:** none at -O2; an unoptimised host build (Debug) would dereference a null part node (PartProjection's constructor sets `m_pNode = 0`; none is constructed in the tree).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `&&`. No play-test.

## `attach` console command -- unbounded `%s` into 64-byte buffers
- **Where:** K2 @0x6e9a74 (`'%s%s%s'` at 0x6e9a9c), K1 @0x5c055c (0x5c0588), into `szGobName` / `szThingName` / `szPartName[64]`.
- **Impact:** a console word over 63 characters overflows the stack.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `%63s%63s%63s`. **Play-test (developer console):** `attach <a 70-character name> x` reports the object not found instead of corrupting the stack.

## K2 `DirtyTextureController::Init` -- 4 bytes per texel into a GetBPP()-sized buffer
- **Where:** K2 @0x6c4958: the image is `new unsigned char[nTS^2 * nTS * GetBPP()]` (0x6c49cc `mul w8, w27, w24; mul w8, w8, w26`, then `new[]`), but the texel loop always writes four bytes (`nDest + nC*4 + 0..3`).
- **Impact:** a 24-bit dirt base texture writes nTS^3 bytes past the buffer (4 KB at the default size 16).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the buffer is sized at max(bpp, 4) bytes per texel (no change for a 32-bit base). **Play-test (K2, optional):** an area with dirty-glass (`proceduretype dirty`) textures looks as before; whether any stock dirt texture is 24-bit was not checked.

## `CResFNT` RLE decoders -- the final packet is not clamped to the image
- **Where:** `ReadColorMappedRLETGA` K2 @0x796a44 / K1 @0x6439c8, `ReadUnmappedRLETGA` K2 @0x796b24 / K1 @0x643ab0: a packet's end `dstPos + count*bpp` is never clamped to `m_tgaSize` (the class `CResPLT`'s gate already fixed; the zero-byte buffer is the entry above).
- **Impact:** a corrupt or truncated final packet writes up to 127 pixels past the decode buffer.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `if (newPos > m_tgaSize) newPos = m_tgaSize;` in the colour-mapped, 24- and 32-bit loops. No play-test (a well-formed stream ends exactly at m_tgaSize; only a malformed .fnt mod changes).

## K2 `PointerMap` -- the block cursor advances 32 times the carved size
- **Where:** port/AURORA/PointerMap.cpp, K2 only: `TakeBlockMemory` @0x814e30 (`add x9, x0, x9, lsl #5`), `CreateMaxTreeNode` @0x814e90 (`add x9, x21, #0xe00` for the 0x70-byte MaxTree), `CreateAABB` @0x8162e0. `m_pCurrent += sizeof(X)` on a 32-byte `PointerMapNode64*` advances 32 x sizeof(X); aurio.cpp's array copies pass byte counts that are also counted as nodes. The block is `new PointerMapNode64[1.5 * mdl size]` (aurio.cpp): 48 bytes of address space per .mdl byte. K1's node is 1 byte, so K1 already packs (the gate excludes it).
- **Impact:** every converted node, tree and AABB starts 32x its own size after the previous one (objects over 128 bytes each on a fresh page, raising RSS per loaded model), and nothing checks the block end, so a model whose converted 64-bit objects exceed 1.5x its .mdl bytes overruns the block.
- **Fixed under `K2_FIX_ENGINE_BUGS` (K2 only, `&& !KOTOR1`):** carves advance by their byte size rounded up to 16 (an array of shorts must not misalign the next object).
- **Play-test (K2, host build) -- ⚠ THIS CHANGES EVERY K2 BINARY MODEL LOAD ON HOST BUILDS:** a regression shows at once as broken, exploded or missing models. Load several modules (Telos, Dxun, Onderon, Nar Shaddaa) and check: area geometry, creatures and skinned meshes (party members, cloaks/robes), animations, placeables and doors, lightsabers and blasters, walkmesh picking (clicking the ground, moving the pointer over objects), and process RSS after an area load (should drop).

## `CAuroraInterface::ReleaseRaster` -- installed in an int-typed function pointer, called with an int for its bool
- **Where:** the `CAuroraInterface` constructor, K2 @0x79a070 (0x79a0d8..0x79a0e0) / K1 @0x63cfec (0x63d054..0x63d05c), stores `ReleaseRaster(void*, int, int, int, int, int, int, bool)` into `AurReleaseRaster`, a `void (*)(..., int)`; `CAurTextureBasic::FreeImage` and the cube loader pass the texture's `unsigned char m_bProcessed` byte through it (a bool-typed pointer changes their code, so the original's pointer is int-typed).
- **Impact:** none observed: every value passed is 0 or 1. Undefined behaviour (a bool parameter outside 0/1) only.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** a thunk `ReleaseRasterThroughInt` converts with `!= 0` before calling ReleaseRaster. No play-test needed (texture release on every area unload is the path; a regression would show as leaked or missing textures).

## `CExoArrayList<T>::Allocate` -- the old count is copied into a smaller array
- **Where:** src/libsource/exobase/_instances/CExoArrayListImpl.h, every instantiation, e.g. K2 `CExoArrayList<CSWSStats_SpellLikeAbility>::Allocate` @0x5f9ff0 (copy bound `ldr w8,[x20,#8]` = m_count); K1 inlines it into `ComputeLastUpdate_StoreUpdateSpellLikeAbility` @0x4de944 (`ldr w8,[x19,#8]` at 0x4de9d8 against the new `w22` elements). `SetSize(n)` (shrink) and `CSWSMessage::ComputeLastUpdate_StoreUpdateSpellLikeAbility` (K2 0x5dea0c: `pDst->Allocate(nCount)` on the previous snapshot) call it with n < m_count. (The AURORA `List<T>` twin is the entry above.)
- **Impact:** a heap overrun when the new size is below the count.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** copy `min(m_count, n)` (all 90 includers code-identical in the matching build). **Play-test (optional, ASan):** switch the controlled party member from a creature with several spell-like abilities to one with fewer; probably unreachable with stock data.

## K1 `CExoString::Format` -- C99 `vsnprintf` truncation is taken for success
- **Where:** K1 @0x650474: it grows only on a negative result (`tbnz w0,#31` at 0x650538), but bionic's (and every host libc's) vsnprintf returns the full length; a result of 0x400 characters or more is then `strncpy`'d at that length out of the 0x400-byte scratch buffer (0x6505dc). K2's `_vsnprintf` shim returns -1, so K2 is unaffected.
- **Impact:** a heap over-read and a garbage tail after byte 1023 of any long formatted string.
- **Fixed under `K2_FIX_ENGINE_BUGS` (K1 arm):** also grows while `nLength >= s_nBufferSize`. **Play-test (K1):** any long formatted text (a long item/feat/power description, a long combat-log or feedback line): no garbage at the end.

## K2 `CompileShader` -- an unchecked fopen, and an off-by-one NUL
- **Where:** port/ASL2/OpenGLES/KotorProgram.cpp, K2 @0x33f958: `bl fopen` -> `fread(buf, 1, 0x20000)` -> `strb wzr,[x24,x22]` with no test on either.
- **Impact:** a missing shader file crashes in fread/fclose; a shader of 128 KB or more writes the NUL one byte past `szSource[0x20000]`.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** return 0 (no shader) on fopen failure; read at most sizeof - 1. No play-test (stock shaders are a few KB; renaming kotor.frag now logs a failed link instead of crashing).

## `CSWSCreatureStats::CalcLevelUpNumberFeats` -- the no-class arm indexes class 0xff
- **Where:** K2 @0x5d0c60 / K1 @0x462c04: `ldrb w9,[x0,#0xbd/#0xb9]` (m_numClasses); `cbz w9` goes to `mov w9,#1; mov w10,#0xff`, then `madd x8, x10, #0x260/#0x1c8, x8` on `g_pRules->m_paClasses` and two `ldrb` (K2 +0x1bd/+0x18b, K1 +0x163/+0x14f) read the normal and bonus counts of "class 255, level 1". The server twin of the client `CSWCLevelUpStats::CalcNumberFeats` entry above.
- **Impact:** a heap over-read about 255 class strides past the classes.2da array (garbage feat counts or a fault). Unreached: both callers (level-up and auto-level) have already read the creature's class slot.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** no class gives 0 normal / 0 bonus feats. No play-test (a creature levels up only through a class slot).

## `CVirtualMachineStack::CopyFromStack` -- the top slice is not bounded by the stack it copies from
- **Where:** K2 @0x7f5f58 / K1 @0x69c61c. The clamp block repeats the base clamp (K2 `0x7f5fec cmp x8, x9 / csel` and again `0x7f6004 cmp x8, w9, sxtw / csel`; K1 0x69c6b0 / 0x69c6c8); nTop gets only `cmp w25, #0x10000` and `> 0`. The memcpy reads `pFrom->m_pTypeData[pFrom->SP - nTop]` for nTop cells.
- **Impact:** a STORE_STATE (via `StackPopCommand`, K2 0x7fa0a4 / K1 0x6a294c) whose local-size operand exceeds the live stack reads before both cell arrays and deep-copies junk as CExoStrings / engine structures. Only a hand-built or corrupt .ncs; the stock compiler emits sizes <= SP - BP. (The two scalar `delete`s of the `new[]` arrays after `ClearStack()` never run: ClearStack nulls both first. The gate spells them `delete[]`.)
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the repeated base clamp becomes `if (nTop > pFrom->m_nStackPointer) nTop = pFrom->m_nStackPointer;`. No play-test (stock scripts take the unchanged path).

## `CVirtualMachineStack::LoadStack` -- the saved BasePointer is never checked
- **Where:** K2 @0x7f644c / K1 @0x69cb18: `str x8,[x20,#8]` (BP) and `str x8,[x20]` (SP) straight from ReadFieldINT, then `cmp w0,#1 / b.lt` on TotalSize; neither pointer is compared with anything (the StackPointer clamp is the earlier entry above).
- **Impact:** a save with BasePointer > StackPointer (or > TotalSize, or negative) makes every BP-relative op (CPDOWNBP / CPTOPBP, CopyFromStack's base slice) index outside the cell arrays; TotalSize < 1 with a non-zero SP leaves both arrays null under a non-zero SP.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** `if (BP > SP) BP = SP;` after the SP clamp, and the TotalSize < 1 return zeroes SP and BP first. No play-test (stock saves carry BP <= SP <= TotalSize; loading a save with a pending DelayCommand exercises the unchanged path).

## `CExoRandInternal::GetString(nID)` -- over-reads the 128-byte ring for nID > 0x80
- **Where:** K2 @0x7a3b94 / K1 @0x64f2a4: `memcpy(pBuf, m_pUnk68, nID)` from the `new unsigned char[0x80]` ring.
- **Impact:** none reachable: the constructor passes 0x10, and `CExoRand::GetString`'s only caller, `CExoEncrypt::GenerateChallenge` (K2 0x79de50 / K1 0x64a25c), has no branch site, no JUMP_SLOT relocation and no caller in src/.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** copies at most the ring and zeroes the rest. No play-test.

## `CFactionManager::DeleteFaction` -- can free faction slot 0, which `SaveFactions` then dereferences
- **Where:** DeleteFaction K2 @0x695470 / K1 @0x3d8424: `ldr x20, [x8, w1, sxtw #3]` with no range check, then `str xzr, [x8, x21, lsl #3]`. SaveFactions K2 @0x694e5c / K1 @0x3d7de8 writes slots 0..m_nNPCFactionCount and reads `m_pArray[i] + 0x20` (FactionName) with no null test. The only caller, `CSWSFaction::RemoveMember`, deletes an emptied faction when the leaver is a PC and `GetIsNPCFaction(id) == 0`; that test is `1 <= id <= m_nNPCFactionCount`, so slot 0 (the player faction) passes.
- **Impact:** a script that moves the PC out of faction 0 while it is the last member there (ChangeFaction / ChangeToStandardFaction) deletes slot 0; every `GetFaction(0)` then returns null and the next save crashes in SaveFactions. No stock script is known to do it (the PC normally sits in its own appended faction, which this path deletes safely).
- **Fixed under `K2_FIX_ENGINE_BUGS`:** DeleteFaction returns for `id <= m_nNPCFactionCount` (the slots SaveFactions writes, negatives included) and for `id >= m_count`. **Play-test (optional, needs a test script):** at the K2 Peragus start (PC alone), run `ChangeToStandardFaction(GetFirstPC(), STANDARD_FACTION_NEUTRAL)` and save: the save completes (the original crashes on save if the PC was in faction 0).

## `CSWRoomSurfaceMesh::ComputeRoomBorder` -- hangs on a border chain that never returns to its start
- **Where:** K2 @0x650e84: the candidate scan `0x65128c add x25, x25, #1 / cmp x26, x25 / b.eq 0x651250` falls back to 0x65125c with the same edge (no exit); K1 @0x556474 the same (0x5568b4 back to 0x556878). It runs only from `LoadMeshText` (an ASCII walkmesh with an aabb section).
- **Impact:** a mod's text .wok whose exterior edges do not form closed rings freezes the module load. Stock rooms ship binary BWM.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** when no continuation is found the ring ends there and its end is recorded in m_aBorderEnds as a closed ring's is. No play-test (stock rooms load from BWM).

## `FuncInterp` / `FuncPreInterp` (aurconsole.cpp) -- the command name is copied unbounded into `szName[128]`
- **Where:** FuncPreInterp K2 @0x70ad58 / K1 @0x5710a4 (K1 `0x5710f8 strb w12, [x9, x8]`, x9 = sp+8; the loop exits only on 0 / ' ' / '='); FuncInterp K2 @0x70a8c4 / K1 @0x570bb0 the same loop.
- **Impact:** a console line, or a console-script line run through AurResGetNextLine, whose first word is 128+ characters overruns the stack frame.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the name stops at 127 characters; the rest reads as argument text. No play-test (developer console or a crafted console script only).

### Found by the ledger, not fixed

- **`ProcessInput` iOS-rotate camera look** (clientinput.cpp:859, `-Wtautological-overlap-compare`): `(fMouseDX <
  0.05f || -0.05f < fMouseDX)` is always true, the orig does both fcmps. Intent is surely `&&` (look only on a
  vertical-dominant drag). Left as the original by the user's decision; the line carries a `(sic)` note.
- **`swvartable`'s `1 << nIndex`** (indices 32..159) was not an engine bug: both originals shift with `lsl w` (mod
  32), and the bitset is word `nIndex >> 5`, bit `nIndex & 31` (no aliasing); the source now says `1u << (nIndex &
  31)` (code-identical, unconditional). `CExoResMan::AddKeyTable`'s probe-slot shift is the same case (the original's
  `lsl w` is mod 32): spelled `static_cast<uint32_t>(static_cast<uint64_t>(1) << (nProbeSlot & 31))`, code-identical.
- **`FModAudioSystem::CreateStream`** stores the stack `szName` as the `FMOD::Sound` user data, which dangles once the
  function returns. Not a defect: nothing reads it back (`FMOD::Sound` declares no `getUserData`; the shims store it
  without dereferencing). No gate.
- **`CExoStringList::Insert` / `Introduce`** (K2 0x7a1594 / 0x7a0ee8, K1 0x651d7c / 0x651698) never check the index.
  Neither has a caller in either game (no branch site, no JUMP_SLOT relocation, nothing in src/). No gate.

## `MdlNodeAnimMesh::InternalGenVertices` — an ASCII animmesh reads its per-frame normals through a null list

- **Where:** `MdlNodeAnimMesh::InternalGenVertices` @0x6d205c (K1 @0x5e8598) reads `m_lAnimNormals`
  (`ldr x9,[x19,#0x210]` at 0x6d2170) once per vertex frame. The ASCII parser
  `MdlNodeAnimMesh::InternalParseField` @0x7686c4 knows only `sampleperiod` / `animverts` /
  `animtverts`; the image has no `animnormals` keyword, so only the binary loader's relocation fills
  the list.
- **Impact:** an ASCII `.mdl` animmesh with vertex frames dereferences `NULL + 12*idx` during post-
  processing. Unreachable with the shipped (all-binary) models; a modder's ASCII model would crash.
- **Confirmation:** both images, same parser and same read. Not fixed: the intended per-frame normal
  source is unknown.

## `BSPTraverseVolumeApprox` — the plane-collapse loop tests an empty vertex list on every node

- **Where:** `BSPTraverseVolumeApprox` @0x7de960 splits each candidate plane against
  `BSPNode::m_lVerts` (`[node,#0x60]`/`#0x68`), but no code in the image ever adds to that list (no
  `List<Vector>::Add` site targets `+0x60`; the ctor zeroes it). The only nodes are the `CAurRoom`s,
  leaf-free with null children.
- **Impact:** none beyond wasted per-frame work — every mask is 0, so the scratch lists stay empty
  and the walk ends at the room. A leftover of a BSP builder the shipped engine does not have.

## `GLRender::RenderObjectsVCQuads` — the VBO arm sets a texcoord pointer where the colour pointer belongs (dead)

- **Where:** `GLRender::RenderObjectsVCQuads` @0x71140c (K1 @0x585bbc). With `GL_COLOR_ARRAY`
  enabled and `GL_TEXTURE_COORD_ARRAY` disabled it calls `glTexCoordPointer(2, GL_FLOAT, 16, 0xc)`
  (0x711490) and never `glColorPointer`, so the colour array would read a stale pointer. The VBO
  arm also draws from 0 instead of `nOffset`.
- **Impact:** none — the function has no caller in either image (no `bl`, no PLT stub). The sibling
  `RenderObjectsVC` shows the intended `glColorPointer(4, GL_UNSIGNED_BYTE, 16, 0xc)`.

## Vertex-pool growth pads by `size & 0xf` instead of rounding up to 16

- **Where:** `PartSkin::PostProcess` @0x6f5c24 (`and w26,w26,#0xf` / `add` at 0x6f6494), and the
  same idiom in the two `MdlNode` vertex-pool growth sites in aurpostprocess.cpp: the new geometry
  starts at `nPoolSize + (nPoolSize & 0xf)` rather than at the next multiple of 16.
- **Impact:** cosmetic — sizes are multiples of 4, so the base stays 4-aligned (enough for GL float
  attributes) and up to 12 bytes are wasted per mesh. The apparent intent (16-byte alignment) is
  not achieved.

## `CSWCModule` — the chase/EQ camera's half-turn yaw clamp is dead: its side flag is only ever 0

- **Where:** `CSWCModule::UpdateCameraModeChaseCam` @0x3d8b0c and `UpdateCameraModeEQ` @0x3d903c test
  `m_nCameraSide` (`+0xbc`) against 2 and 1 to snap a >90° yaw difference to ∓90°. The only stores
  to `+0xbc` in the image are zeros (the ctor @0x38fa0c, `WalkPlayerToPoint` @0x3d6ea4).
- **Impact:** none observable — the clamp never fires. The writer that would pick a side is absent
  from the shipped engine.

## `CSWCCreature::UpdateAttackData` — the `g_nCombatAnimation` debug override can never fire

- **Where:** `CSWCCreature::UpdateAttackData` @0x496c58: `if (g_nCombatAnimation == 0 ||
  m_objectId != 0xffffffff)` takes the normal path (`cmp x8,x9` with `x9 = 0xffffffff` at
  0x496cf8). Client object ids are never 0xffffffff, and nothing in the image writes the global
  (its only GOT use is this read at 0x496ce4).
- **Impact:** none — a developer hook (set from a debugger) whose guard is inverted or stale.

## `CSWCAnimBase::ServerToClientAnimation` — two-handed and dual sabers play each other's damage flinch (and K1 never plays deflect 1 for them)

- **Where:** `CSWCAnimBase::ServerToClientAnimation` @0x361e88 (K1 @0x24cf20). For
  `SERVER_ANIMATION_DAMAGE_GENERIC` (10302) the weapon-class jump table (K2 @0x9bd9cc, K1 @0x6c746d)
  sends class 3 (two-handed saber) to row 206 `g4d1` (DS_Damage_Generic_Attack) and class 4 (dual
  saber) to row 165 `g3d1` (2HS_Damage_Generic_Attack). K1 additionally maps
  `SERVER_ANIMATION_DEFLECT_1` (10300) for classes 3/4 to the deflect-2 rows 151 `c3n2` / 192 `c4n2`
  (packed table `0x6d/0x97/0xc0` at 0x24d648); K2 already uses 150 `c3n1` / 191 `c4n1` there.
- **Impact:** a hit on a two-handed or dual saber wielder plays the other style's flinch (wrong
  stance for the held weapon); in K1 those two styles only ever show their second deflect.
- **Confirmation:** both images decode the same damage table; the K2 deflect arm is the developers'
  own correction of the K1 one, and K1's s_female02 supermodel carries `c3n1`/`c4n1`.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** damage class 3 → `g3d1`, class 4 → `g4d1`; K1 deflect 1 →
  `c3n1`/`c4n1`.

## `CSWCCreatureAppearance::SetAppearanceAlignment` — body variant N looks up a `TEXMEVIL` column that does not exist

- **Where:** `CSWCCreatureAppearance::SetAppearanceAlignment` @0x3d5850 (K2): the armour arm for
  `m_cachedBTypeVariant == 14` (`cmp w8,#0xe` at 0x3d5c0c) fetches `"TEXMEVIL"` (0x3d5c38). Variant
  letters are `variant | 0x40` (14 = 'N'), and appearance.2da's columns are `texaevil`, `texbevil`,
  `texievil`, `texlevil`, `texnevil` — there is no `texmevil`.
- **Impact:** the lookup always fails, so an N-variant body never gets its dark-side texture. The
  only row that fills `texnevil` is 454 `Party_NPC_Handmaiden` (`P_HandmaidenAD`): in that body she
  never shows her dark-side texture, while her B variant does.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** reads `TEXNEVIL`.

## `CSWMGGunBank::SetBulletTargetType` — the validity check is a hex-digit mask over a bit-mask value

- **Where:** `CSWMGGunBank::SetBulletTargetType` @0x5068ac rejects `nType & 0x11111110` (`tst w1,w9`
  at 0x5068d8). Target types are bits (player 1, enemy 2 — `CSWTrackFollower::TargetType`), so the
  check only catches values with bit 4/8/…/28 set; 4, 6, 0x20 etc. are accepted.
- **Impact:** none with shipped data (1–3); an invalid value is stored and simply never matches a
  follower. Probably meant `nType & ~3`.

## Client visual effects — NWN-era ids the KOTOR data never supplies

- **Where:** `CSWCVisualEffectOnObject::ApplyMirv` @0x3f7724 tags its projectile with spell 340
  (`mov w1,#0x154` at 0x3f77e4) — beyond both spells.2da tables (K2 282 rows, K1 132), so
  `SetProjectileSpellId` is a no-op; and no visualeffects.2da `progfx_*` value reaches the MIRV band
  (900–999) at all. `CSWCVisualEffect::SetPosition` @0x3802f4 / `LoadEffect` / `SetAnimation`
  special-case effect ids 39/40, but visualeffects.2da labels start at 1001 in both games.
  `VFX_DUR_STEALTH_PULSE` (2000) and `VFX_DUR_INVISIBILITY` (2001) carry `progfx_duration` 2000/2001,
  which `ApplyHardCodedVisualEffect` @0x3f5174 (bands 100–1899) does not handle.
- **Impact:** none — dead branches and no-op effects inherited from NWN; the two stealth effects act
  only as markers.

## Attack-roll feedback — the concealment lines (results 16/17) cannot be sent

- **Where:** the result code travels as a 4-bit field (`WriteBYTE(…,4)` server side, `ReadBYTE(4)`
  at 0x3ab024 in `CSWCMessage::HandleServerToPlayerCCMessage`), yet the client gates the concealment
  field and strings on `(result & 0xfe) == 16` (0x3ab0a4, 0x3ab710). No server path produces 16/17
  either.
- **Impact:** none — a dead NWN feature; KOTOR never reports concealment rolls.

## `CSWCCreatureStats::GetDamageBonusString` — every damage-type name is strref 0 ("Bad StrRef") (dead)

- **Where:** `CSWCCreatureStats::GetDamageBonusString` @0x3708d4 (K1 @0x283154) fetches strref 0 in all
  15 damage-subtype arms and in the `m_nFlag` prefix; only the alignment words use real strrefs.
- **Impact:** none — the function has no caller in either image. Placeholder strings never filled in.

## `verifyallcreatures` (debug console) — the sit/stand skip is in the wrong loop, and the census prints nothing

- **Where:** `verifyallcreatures` @0x4e4bac applies the "skip rows 20 and 22 for non-full creatures"
  test (`orr w8,w24,#2; cmp w8,#0x16` at 0x4e4d58) inside the loop over rows 200–270, where it can
  never match; the 0–87 loop it was meant for has none. Every missing-animation name it builds is
  discarded.
- **Impact:** none — a developer console command with no output.

## Server-applied visual effects — NWN visualeffects ids with no KOTOR row

- **Where:** the blood-colour branches (`VFX_COM_BLOOD_RED/GREEN/YELLOW` 158–160) and
  `OnApplyDisease` (51) in nwseffectlist.cpp, the disease payload (84, nwscreature.cpp), the on-hit
  property visuals (50, 82, 91, 94, 95, 207, 208, 218, 232, nwscreatureaicombat.cpp) and
  `GetReflexAdjustedDamage`'s save flash (4000, nwvmachinecommands.cpp). Visual effects resolve by
  row LABEL, and both games' visualeffects.2da start at 1001 with no 4000 (3019 → 4003).
- **Impact:** those visuals never play; the lookups fail harmlessly. No clear replacement rows, so
  not fixed.

## `CPathfindInformation`'s forced-move deadline is never armed (and two sibling slots are never set)

- **Where:** `CSWSCreature::WalkUpdateLocation_QuickWalk` @0x5463b0 tests
  `m_pPathfindInfo->m_bForceMoveDeadline` (+0x334, `ldr` @0x546550) and, when it is set and the
  deadline day/time (+0x338/+0x33c) has passed, logs "Force Timeout, executing a forced move to
  point." and calls `ForceMoveToPoint`. No store in the image ever puts a nonzero value in
  +0x334/+0x338/+0x33c; every write is a reset to 0. The same holds for
  `CPathfindInformation::m_bResendPath` (+0x2f8, only cleared, read by the client-path resend test
  in `nwssendplayermessage.cpp`) and `m_oidIgnoredBlocker` (+0x40, only ever `OBJECT_INVALID`, read
  as an ignore id by `CSWSArea::NoNonWalkPolysDetailed`).
- **Impact:** none observable. The timeout safety net for a stuck walk is dead code, so a creature
  whose pathing stalls never gets the forced move. The deadline's length is not recorded anywhere,
  so there is no clear value to arm it with.
- **Confirmation:** a full-image scan for nonzero stores at those offsets (immediate and add-base
  forms) finds no pathfinding or creature writer. The source's writes are all 0/OBJECT_INVALID.

## `CSWSObject::m_bMovedByPowerOrFeat` is never set, so `GetCreatureMovedByPowerOrFeat` always answers 0

- **Where:** `CServerExoApp::GetCreatureMovedByPowerOrFeat` @0x5fc550 (K1 @0x53dc2c) returns 1
  and the creature's position only when `+0x2fc` (K1 `+0x2d0`) is nonzero, then clears it. Neither
  image has a store of a nonzero value to that CSWSObject offset. The one `CSWSCreature` store at
  that displacement (@0x545288, `AIActionMoveToPoint`) writes through `m_pPathfindInfo`, so it is a
  different field.
- **Impact:** none observable. The position resync after a Force push or feat knockback that this
  answer would drive is dead. The client gets positions through the normal update path.
- **Confirmation:** a full-image store scan of both binaries.

## Item on-hit spell impacts are never queued (`m_onHitSpellImpacts` has no producer)

- **Where:** `CSWSCreature::SignalMeleeDamage` / `SignalRangedDamage` drain
  `CSWSCombatAttackData::m_onHitSpellImpacts` (+0xb8) into `EVENT_ITEM_ON_HIT_SPELL_IMPACT` events,
  and the creature event handler reads `CSWSSpellScriptData::m_nCasterLevel` (+0x48) for them. Nothing in
  either image ever adds to that list (no `CExoArrayList<CSWSSpellScriptData*>` code at all, no
  nonzero count store), and nothing writes, saves or loads `m_nCasterLevel`.
- **Impact:** none. This is a vestigial NWN on-hit "cast spell" path. KOTOR's on-hit properties go
  through the effect lists instead.
- **Confirmation:** dynsym and store scans of both images.

## `SpawnInHeartbeatPerception` never restamps its perception timer, so the 4-second throttle never applies

- **Where:** `CSWSCreature::SpawnInHeartbeatPerception` @0x625654 (K1 @0x404cf8). Bucket C
  measures `now - (m_nLastPerceptionDay, m_nLastPerceptionTod)` (+0x55c/+0x560, K1 +0x51c/+0x520)
  and runs `UpdateVisibleList(1)` once that exceeds 4000 ms for an NPC or `m_nPerceptionDelay` for
  a PC. The pair is zeroed by the constructor and never written again. The other two buckets of
  the same function restamp their own day/time pairs.
- **Impact:** every heartbeat-driven call performs the full visible-list pass for every creature
  not in the party, instead of once per 4 s (NPC) or 0.3-0.7 s (PC). It costs CPU, and it is also
  part of the shipped NPC reaction speed. Related: `m_bSuppressPerceptionEvents` (+0x17d0) is
  only ever cleared (@0x55086c), so the suppression it would provide never happens.
- **Confirmation:** full-image store scans (K2 and K1) find no CSWSCreature writer of the pair.
  Not fixed, because restoring the throttle would change shipped AI responsiveness.

## `GetForceItemMask` looks up equipment by slot INDEX where `GetItemInSlot` takes a slot BIT

- **Where:** `CSWSCreature::GetForceItemMask` @0x63f8c0 (K1 @0x424420) calls
  `CSWInventory::GetItemInSlot(i)` for i = 0..19 (K1 0..17). `GetItemInSlot` maps single slot
  bits, so only i = 1/2/4/8/16 resolve: head, body, boots, hands and right weapon.
- **Impact:** small. Armour (body) and the right-hand weapon are still seen, so the armour-class
  forbid bits and the lightsaber require bit work in the usual case. A lightsaber that is only in
  the left hand or a creature weapon slot does not satisfy a power's lightsaber requirement.
  The mask feeds `AIActionCastSpell`'s spells.2da forbid/require masks and the client's radial-menu
  force lists.
- **Confirmation:** same loop bound and argument in both games. Not fixed: iterating `1u << i`
  would also count the inactive weapon set and the creature slots, so the intended set is unclear.

## `OnRemoveTrueSeeing` rebuilds the ULTRAVISION bit instead of TRUE SEEING

- **Where:** `CSWSEffectListHandler::OnRemoveTrueSeeing` @0x6676d0 (K1 @0x47db98). It clears
  `VISION_TRUE_SEEING` (bit 4) and, for every other live true-seeing effect, ORs bit **2**
  (`VISION_ULTRAVISION`, `orr w15,w11,#2` @0x66775c, K1 @0x47dc2c). It is a copy of
  `OnRemoveUltraVision` with the bit left unchanged.
- **Impact:** with two true-seeing effects on a creature, removing one strips true seeing
  although the other is still active, and grants ultravision instead. Darkness and invisibility
  then apply to a creature that should see through them.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the rebuild ORs `VISION_TRUE_SEEING`.

## `OnRemoveForceResistanceDecrease` rebuilds the penalty from the INCREASE effects

- **Where:** `CSWSEffectListHandler::OnRemoveForceResistanceDecrease` @0x665948 (K1 @0x47ba40).
  The rebuild loop walks subtypes 33..34 and keeps the largest value of every effect that is
  **not** a decrease (`cmp w24,#0x22; b.eq skip` @0x6659e4), then hands it to
  `SetForceResistancePenalty`. The apply side stores the largest DECREASE as the penalty.
- **Impact:** when a Force-resistance decrease expires on a creature that also has a resistance
  increase (an item or power), the creature is given a penalty equal to that increase, which
  cancels its bonus, instead of having the penalty cleared. If no increase exists the result is 0,
  so a second, still-active decrease is also lost.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the rebuild takes the largest surviving DECREASE.

## Item use provokes attacks of opportunity only for stimulants (the second type test repeats STIMULANT)

- **Where:** `CSWSCreature::AIActionItemCastSpell` @0x53f034 (K1 @0x428a00). At the first tick of
  an item cast it calls `BroadcastAttackOfOpportunity` when the item type is STIMULANT (25) **or**
  STIMULANT: the two `cmp w8,#0x19` @0x53f7b8/@0x53f7cc (K1 @0x4290e4/@0x4290f8).
- **Impact:** using a medpac or other usable item never provokes the attack of opportunity a stim
  does. It is unknown which type the second test meant, so this is not fixed.

## `GetItemComponent` / `GetChemical` on a placeable answer its CREDITS pile

- **Where:** `CSWVirtualMachineCommands::ExecuteCommandGetItemComponent` @0x58f818 (@0x58f89c) and
  `ExecuteCommandGetChemical` @0x5778cc (@0x577950). For a placeable target both search the
  repository with `FindItemWithBaseItemType(ITEM_TYPE_CREDITS /*23*/, 0)`, copied from
  `ExecuteCommandGetGold` @0x577700. Components and chemicals are item types 51 and 50
  (baseitems.2da rows 96/95), which `CItemRepository::AddItem` handles as those counters.
- **Impact:** a script asking how many components or chemicals a container holds gets its credit
  count (or 0). K2 only.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** the searches use `ITEM_TYPE_COMPONENTS` / `ITEM_TYPE_CHEMICALS`.

## The door/placeable KNOCK action casts spells.2da row 93 — NWN's Knock, KOTOR's cryoban grenade

- **Where:** `ExecuteCommandDoDoorAction` @0x56a8a4 and `ExecuteCommandGetIsDoorActionPossible`
  @0x5795bc test and cast spell 93 for `DOOR_ACTION_KNOCK`. In KOTOR row 93 is
  ITEM_ABILITY_GRENADE_CRYOBAN (0 Force points, so `GetSpellUsesLeft` answers 1).
- **Impact:** latent. A script doing `DoDoorAction(oDoor, DOOR_ACTION_KNOCK)` would cast the
  cryoban effect at the door, and `GetIsDoorActionPossible(…, DOOR_ACTION_KNOCK)` is TRUE for any
  unlocked, non-plot, no-key door. The shipped BIF scripts only use OPEN and BASH (`k_ai_master`).
  KOTOR has no Knock spell to substitute, so this is not fixed.

## `StartServices` deletes "HD0:TEMP" with resource type 86, which does not exist — a no-op

- **Where:** `CServerExoAppInternal::StartServices` (K2 @0x6067a4, K1 @0x5493ec):
  `g_pExoResMan->RemoveFile("HD0:TEMP", 0x56)`. Type 86 is not in the resource-extension table,
  so the extension is empty and the path is the TEMP directory, which `RemoveFile` refuses.
- **Impact:** none. Whatever leftover file this was meant to clear is not cleared. Intent unknown.

## Several client feedback messages fetch strref 0 ("Bad StrRef")

- **Where:** `CClientExoAppInternal::AddFeedbackMessage` @0x478f80. Ids 4, 5, 47, 69, 70, 71 and 222
  set their custom tokens and then fetch strref **0** (`mov x1,xzr` before the inlined
  `GetGUIString`, e.g. @0x47bad4 for id 4, @0x47a254 for id 222). Ids 2, 3, 8 and 223-226 do the same
  without tokens.
- **Impact:** latent. Such a message would print dialog.tlk entry 0, "Bad StrRef", in the feedback
  log. No KOTOR server path is known to send these ids (NWN feedback residue), and the intended text
  is not in the data, so this is not fixed.

## `CSWGuiSwoopUpgrade` — the lightsaber/armour menu branches are dead (`m_nMenuType` is always 0)
- **Where:** `CSWGuiSwoopUpgrade` (+0x5b49). Only the constructor and `OnPanelRemoved` @0x4cf578 store it
  (`strb wzr`). Nothing else in the image does, and `CGuiInGame::ShowSwoopUpgradeScreen` @0x35591c never sets
  it. The `== 1` (3D_MODEL_LS) and `== 4` (armour) branches in OnSlotSelected, ShowItems, SetDescription,
  Update3DView, OnUpgradeSelected and HandleInputEvent are copied from `CSWGuiUpgrade` and never run.
- **Impact:** none. The swoop-upgrade screen itself has no in-game trigger in KOTOR II's data
  (command 785 appears only in the unreferenced `k_ptar_swoopup`).

## `CSWGuiControllerLossBox::OnPanelAdded` — KOTOR II shows "Component Value" when the controller is lost
- **Where:** `CSWGuiControllerLossBox::OnPanelAdded` @0x3e4e60 (`mov w1, #0xbd0e` @0x3e4ec4). Strref 48398 is
  KOTOR 1's "Please reconnect the controller and press the Enter key to continue." (K1 @0x31cff8 passes the
  same number). KOTOR II's dialog.tlk spends 48398 on "Component Value". Its own reconnect line, 42130
  ("Please reconnect the controller and press START to continue."), is never referenced.
- **Impact:** when `CExoInput::GetMainDeviceLost()` fires (`UpdateControllerPauseState` ->
  `EnterControllerPauseState`), the non-cancellable pause box reads "Component Value".
- **Fixed under `K2_FIX_ENGINE_BUGS`:** KOTOR II passes 42130 (`STRREF_CONTROLLER_DISCONNECTED_K2`). KOTOR 1 is
  unchanged.

## `CGuiInGame::GetCameraAnimationName` — camera ids 1028 / 1228 name "cut039" / "cut039w" instead of "cut029"
- **Where:** `CGuiInGame::GetCameraAnimationName`. The 1000-series table runs cut001..cut039 in order except
  that 1028 maps to "cut039" (as 1038 does). The "w" series repeats it at 1228. No image (K1/K2 Android, K1/K2
  Windows) contains a "cut029" string.
- **Impact:** none in shipped data (dialog camera models only carry cut001-cut010 in KOTOR II and cut001-cut011
  and cut040 in KOTOR 1). A custom camera model with a cut029 animation can never be played by id 1028.
- **Not fixed:** every PC release has the same mapping, so mods may depend on it.

## `CSWGuiInGameInventory::NotUseableOnHawk` (KOTOR 1) — an empty message box
- **Where:** K1 `CSWGuiInGameInventory::NotUseableOnHawk` @0x341f60 (`mov w1, #0xbff9` @0x341fa0). It is
  registered for INPUTEVENT_GUI_A on every item row while `m_nNPC != -1` in a tutorial module. Strref 49145 is
  empty in every shipped KOTOR 1 dialog.tlk (PC; Android dialog/tv_dialog in all five languages; Switch).
- **Impact:** pressing A on an item in that state shows a blank non-cancellable box.
- **Not fixed:** no string is clearly the intended one (1434 "You cannot use that item." is a guess).

## `CSWGuiCreateItem` / `CSWGuiCreateMedicalItem::OnControlEntered` — infinite stock shows "41951"
- **Where:** @0x4c43f0 and @0x4c96b8: when the party repository's matching item is infinite
  (`GetMatchingItemCount` returns -1), the stock label gets `SetText(CExoString(41951))`, which is the number
  rendered as text. The branch beside it (an infinite item outside the repository, @0x4c4364 / @0x4c962c) calls
  `SetStrRef(41951)`, which shows "Infinite".
- **Impact:** the workbench / lab station shows "41951" as the stock of an infinite repository item.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** both sites call `SetStrRef(STRREF_INFINITE)`.

## `CSWGuiSwoopUpgrade::GetArmorModelAndTextureResRef` — checks for a .txb where its twin checks .tpc
- **Where:** @0x4cf914 (`mov w2, #0xbbe` @0x4cfa90, RESTYPE_TXB). `CSWGuiUpgrade::GetArmorModelAndTextureResRef`
  @0x4be390 tests RESTYPE_TPC (`#0xbbf`). A TPC-only texture would fall back to the model's `%s01`.
- **Impact:** none. The only caller is `Update3DView`'s `m_nMenuType == 4` branch, which never runs (see the
  swoop `m_nMenuType` entry), on a screen with no in-game trigger.

## `CSWGuiSaveLoad::CorruptGame` — KOTOR II's message strref is empty (unreachable)
- **Where:** @0x452cec (`mov w1, #0xbc51` @0x452d30). 48209 is KOTOR 1's "Unable to load saved game." and is
  empty in KOTOR II's dialog.tlk.
- **Impact:** none. No relocation or call in either image references the function, so it is never registered.

## `UpdateDriveMode` — the drive-key "tap" test reads a press time nothing ever records

- **Where:** `CClientExoAppInternal::UpdateDriveMode` @0x38d37c (K1 @0x22d580): `ldr x8,[x19,#0x198];
  sub x8,x22,x8; cmp x8,#0x31; b.hi` tests `now - m_nDriveKeyPressTime <= 49` µs before stepping the
  player half a unit back along its facing (`SendPlayerToServerInput_WalkToWayPoint`). The only stores
  to the field in either game are zeroes: the constructor, `SetInputActive` @0x47ed90 and
  `TurnOffKeyboardModal` @0x47ee2c (K1 @0x2246ec / @0x224788).
- **Impact:** none visible. The tap branch is dead code because the snapshot time is never within 49 µs of 0,
  so a forward/run key press while in AI state 2 always takes the normal drive path.
- **Confirmation:** a whole-binary store search for +0x198 (K2) / +0x190 (K1) finds no writer of a
  time value. Document only: no build shows when the press time was meant to be recorded.

## `CWin32MSGQueue::CWin32MSGQueue` — the hot-key counter is left uninitialised

- **Where:** `CWin32MSGQueue::CWin32MSGQueue` @0x874a50 zeroes every message-class counter at
  +0x68..+0x90 except +0x8c (`m_nHotKeyCount`): the stores are `stur q0` +0x68/+0x78, `str wzr` +0x88
  and +0x90, with nothing at +0x8c.
- **Impact:** none in practice. Only `AdjustStatus`'s `WM_HOTKEY` case reads the counter (it decides
  when to clear the QS_HOTKEY status bit), and nothing posts `WM_HOTKEY` to the ASL queue.
- **Confirmation:** constructor disassembly above. Document only.

## `ThreadHANDLE::WaitForPendingIO` — dereferences the popped entry before testing it for null

- **Where:** `ThreadHANDLE::WaitForPendingIO` @0x870a18: `ldr x8,[x0,#0x20]; str xzr,[x8,#0x8]`
  (0x870a40) runs before `cbz x0` (0x870a48), so the null check after `InterlockedPopEntrySList`
  is dead and an empty list would crash.
- **Impact:** none. The function has no callers in the shipped library, and the pending-IO list is never
  filled (`SignalCompletedIO` is empty).
- **Confirmation:** disassembly above. Document only.

## `Achievements::EvaluateAchievement` — the Peragus escape line also unlocks "Dancing Queen"

- **Where:** `Achievements::EvaluateAchievement` @0x6c0558. The case for strref 112717 ("All right,
  let's get out of here!", the Peragus turret escape) unlocks achievement 35 (Don't Get Cocky) when
  the run is clean, then falls through (0x6c0b30, `mov w3,#0x28`) into the case for strref 112148 (Vogga's
  majordomo, "…convey his compliments…") and unlocks achievement 40, Dancing Queen. It is a missing `break`:
  112148 branches straight to the same 0x6c0b30.
- **Impact:** every player who finishes the Peragus turret escape unlocks "Dancing Queen", whether or not
  the cocky run succeeded. On the desktop build that unlock is reported to Steam.
- **Confirmation:** branch layout above; TLK text for 112717 / 112148 / 112721. **Fixed under
  `K2_FIX_ENGINE_BUGS`**: the 112717 case unlocks only Don't Get Cocky.

## K1 `CAurTriangleBin::RenderGrassPolys` — grass never fades in on the GLSL renderer

- **Where:** KOTOR 1's `CAurTriangleBin::RenderGrassPolys` (Android libKOTOR.so @0x42a064). A bin
  between `m_fNearRadiusSquared` (15 m) and `m_fFarRadiusSquared` (30 m) gets a fade alpha, which the
  function hands to `glMaterialfv(GL_DIFFUSE)` and, only inside `if (!AurNonRadeon8500Validate())`
  (an ATI Radeon 8500), to vertex-program env parameter 94. KOTOR II sets 94/95 on every card.
- **Impact:** the mobile renderer (KotorProgram) takes the material from env parameters 93-95 and
  never reads `glMaterialfv`, so on any other GPU the grass draws with the previous mesh's diffuse
  alpha (opaque) and pops in whole at the 30 m edge instead of fading in over 15-30 m (reported on
  Dantooine, K1 xbox on Windows, 2026-10-24). The PC's fixed-function path read the material and faded.
- **Fixed under `K2_FIX_ENGINE_BUGS`:** env parameters 94 (grass diffuse, fade alpha) and 95 (grass
  ambient) are also set on the other cards, as KOTOR II does.
