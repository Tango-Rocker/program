#include <stdio.h>

#include "colony/inventory.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[inventory] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_inventory(void) {
    int failed = 0;

    GameInventory inventory = {0};
    GameInventoryEntry entries[8] = {0};
    GameInventoryReservation reservations[8] = {0};

    failed += assert_true(
        game_inventory_init(&inventory, entries, 8u, reservations, 8u, 10u) == GAME_INVENTORY_RESULT_OK,
        "inventory init"
    );

    GameInventoryOwner owner_alpha = {GAME_INVENTORY_OWNER_KIND_ENTITY, {10u, 1u}, 0u};
    GameInventoryOwner owner_beta = {GAME_INVENTORY_OWNER_KIND_ENTITY, {11u, 1u}, 0u};
    GameInventoryOwner owner_stockpile = {GAME_INVENTORY_OWNER_KIND_STOCKPILE, {0u, 0u}, 77u};

    GameInventoryEntryHandle alpha_res100 = {0u, 0u};
    GameInventoryEntryHandle alpha_res200 = {0u, 0u};

    failed += assert_true(
        game_inventory_add(&inventory, owner_alpha, 100u, 8u, &alpha_res100) == GAME_INVENTORY_RESULT_OK,
        "add owner alpha resource 100"
    );
    failed += assert_true(
        game_inventory_add(&inventory, owner_alpha, 200u, 2u, &alpha_res200) == GAME_INVENTORY_RESULT_OK,
        "add owner alpha resource 200"
    );
    failed += assert_true(
        game_inventory_add(&inventory, owner_beta, 100u, 4u, NULL) == GAME_INVENTORY_RESULT_OK,
        "add owner beta resource 100"
    );
    failed += assert_true(
        game_inventory_add(&inventory, owner_stockpile, 5u, 3u, NULL) == GAME_INVENTORY_RESULT_OK,
        "add stockpile owner resource 5"
    );

    GameInventoryEntryView all[8] = {0};
    size_t all_count = game_inventory_dump_all(&inventory, all, 8u);
    failed += assert_true(all_count == 4u, "dump all inventory entry count is deterministic");
    failed += assert_true(all[0].owner.kind == GAME_INVENTORY_OWNER_KIND_ENTITY
        && all[0].resource_id == 100u
        && all[0].handle.slot == alpha_res100.slot,
        "dump all first entry retains insertion order"
    );
    failed += assert_true(
        all[1].owner.kind == GAME_INVENTORY_OWNER_KIND_ENTITY && all[1].resource_id == 200u
            && all[1].handle.slot == alpha_res200.slot,
        "dump all second entry preserves deterministic order"
    );
    failed += assert_true(all[2].owner.kind == GAME_INVENTORY_OWNER_KIND_ENTITY && all[2].resource_id == 100u,
        "dump all includes owner beta third");
    failed += assert_true(all[3].owner.kind == GAME_INVENTORY_OWNER_KIND_STOCKPILE && all[3].owner.stockpile_id == 77u,
        "dump all includes stockpile ownership");

    GameInventoryEntryView alpha_view[8] = {0};
    size_t alpha_count = game_inventory_dump_owner(&inventory, owner_alpha, alpha_view, 8u);
    failed += assert_true(alpha_count == 2u, "owner alpha query returns two resources");
    failed += assert_true(
        alpha_view[0].resource_id == 100u && alpha_view[1].resource_id == 200u,
        "owner alpha query is in deterministic slot order"
    );

    GameInventoryReservationHandle alpha_reservation = {0u, 0u};
    failed += assert_true(
        game_inventory_reserve(&inventory, owner_alpha, 100u, 4u, &alpha_reservation) == GAME_INVENTORY_RESULT_OK,
        "reserve half of owner alpha resource"
    );
    failed += assert_true(game_inventory_total_reserved(&inventory, owner_alpha, 100u) == 4u,
        "reserved quantity tracks reservation state");

    uint32_t removed = 0u;
    failed += assert_true(
        game_inventory_remove(&inventory, owner_alpha, 100u, 1u, &removed) == GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY,
        "remove blocked while reservation is active"
    );
    failed += assert_true(removed == 0u, "no quantity removed while reserved");

    failed += assert_true(
        game_inventory_reserve(&inventory, owner_alpha, 100u, 5u, NULL) == GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY,
        "second reservation beyond availability rejected"
    );

    failed += assert_true(
        game_inventory_commit_reservation(&inventory, alpha_reservation, NULL) == GAME_INVENTORY_RESULT_OK,
        "commit reservation succeeds"
    );
    failed += assert_true(
        game_inventory_total_reserved(&inventory, owner_alpha, 100u) == 0u,
        "reservation quantity clears after commit"
    );

    failed += assert_true(
        game_inventory_remove(&inventory, owner_alpha, 100u, 3u, &removed) == GAME_INVENTORY_RESULT_OK,
        "remove after commit works"
    );
    failed += assert_true(removed == 3u, "removed committed amount");

    failed += assert_true(
        game_inventory_add(&inventory, owner_alpha, 100u, 10u, NULL) == GAME_INVENTORY_RESULT_OUT_OF_SPACE,
        "stack overflow rejected"
    );

    GameInventoryReservationHandle stockpile_reservation = {0u, 0u};
    failed += assert_true(
        game_inventory_reserve(&inventory, owner_stockpile, 5u, 1u, &stockpile_reservation)
        == GAME_INVENTORY_RESULT_OK,
        "stockpile owner reservation succeeds"
    );
    failed += assert_true(game_inventory_release_reservation(&inventory, stockpile_reservation) == GAME_INVENTORY_RESULT_OK,
        "release reservation restores availability");

    if (failed == 0) {
        printf("[inventory] PASS\n");
    }
    return failed;
}
