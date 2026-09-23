#include <string>

#include <substate/MemoryStorageEngine.h>

#include <boost/test/unit_test.hpp>

using namespace ss;

BOOST_AUTO_TEST_SUITE(test_MemoryStorageEngine)

namespace {

    Transaction transaction(int n) {
        std::map<std::string, std::string> message;
        message["n"] = std::to_string(n);
        return Transaction({}, std::move(message));
    }

    std::string messageOf(const MemoryStorageEngine &engine, int step) {
        auto message = engine.stepMessage(step);
        auto it = message.find("n");
        return it == message.end() ? std::string() : it->second;
    }

    std::string messageOf(const Transaction *transaction) {
        return transaction ? transaction->message().at("n") : std::string();
    }

    void checkSteps(const MemoryStorageEngine &engine, int minimum, int current, int maximum) {
        BOOST_CHECK_EQUAL(engine.minimumStep(), minimum);
        BOOST_CHECK_EQUAL(engine.currentStep(), current);
        BOOST_CHECK_EQUAL(engine.maximumStep(), maximum);
    }

}

BOOST_AUTO_TEST_CASE(test_an_empty_history_has_no_step) {
    MemoryStorageEngine engine;
    checkSteps(engine, 0, 0, 0);
    BOOST_CHECK(!engine.stepBackward());
    BOOST_CHECK(!engine.stepForward());
    BOOST_CHECK(engine.stepMessage(0).empty());
}

BOOST_AUTO_TEST_CASE(test_each_commit_is_one_step) {
    MemoryStorageEngine engine;
    for (int n = 1; n <= 3; ++n) {
        engine.commit(transaction(n));
    }
    checkSteps(engine, 0, 3, 3);
    BOOST_CHECK_EQUAL(messageOf(engine, 1), "1");
    BOOST_CHECK_EQUAL(messageOf(engine, 3), "3");
    BOOST_CHECK(engine.stepMessage(0).empty());
    BOOST_CHECK(engine.stepMessage(4).empty());
}

BOOST_AUTO_TEST_CASE(test_stepping_returns_the_transaction_of_the_step_left_or_entered) {
    MemoryStorageEngine engine;
    for (int n = 1; n <= 3; ++n) {
        engine.commit(transaction(n));
    }

    BOOST_CHECK_EQUAL(messageOf(engine.stepBackward()), "3");
    BOOST_CHECK_EQUAL(messageOf(engine.stepBackward()), "2");
    checkSteps(engine, 0, 1, 3);

    BOOST_CHECK_EQUAL(messageOf(engine.stepForward()), "2");
    checkSteps(engine, 0, 2, 3);

    BOOST_CHECK_EQUAL(messageOf(engine.stepForward()), "3");
    BOOST_CHECK(!engine.stepForward());
    checkSteps(engine, 0, 3, 3);
}

BOOST_AUTO_TEST_CASE(test_a_commit_truncates_the_undone_steps) {
    MemoryStorageEngine engine;
    for (int n = 1; n <= 3; ++n) {
        engine.commit(transaction(n));
    }
    engine.stepBackward();
    engine.stepBackward();

    engine.commit(transaction(4));
    checkSteps(engine, 0, 2, 2);
    BOOST_CHECK_EQUAL(messageOf(engine, 1), "1");
    BOOST_CHECK_EQUAL(messageOf(engine, 2), "4");
    BOOST_CHECK(!engine.stepForward());
}

BOOST_AUTO_TEST_CASE(test_the_oldest_steps_are_evicted_beyond_the_limit) {
    MemoryStorageEngine engine(3);
    for (int n = 1; n <= 5; ++n) {
        engine.commit(transaction(n));
    }
    checkSteps(engine, 2, 5, 5);
    BOOST_CHECK(engine.stepMessage(2).empty());
    BOOST_CHECK_EQUAL(messageOf(engine, 3), "3");
    BOOST_CHECK_EQUAL(messageOf(engine, 5), "5");

    // Exactly the limit remains reachable by undo.
    for (int n = 5; n >= 3; --n) {
        BOOST_CHECK_EQUAL(messageOf(engine.stepBackward()), std::to_string(n));
    }
    BOOST_CHECK(!engine.stepBackward());
    checkSteps(engine, 2, 2, 5);
}

// A limit of 1 was rejected silently by the previous engine, which accepted only 4 or more.
BOOST_AUTO_TEST_CASE(test_a_limit_of_one_step_is_accepted) {
    MemoryStorageEngine engine(1);
    engine.commit(transaction(1));
    engine.commit(transaction(2));
    checkSteps(engine, 1, 2, 2);
    BOOST_CHECK_EQUAL(messageOf(engine.stepBackward()), "2");
    BOOST_CHECK(!engine.stepBackward());
}

BOOST_AUTO_TEST_CASE(test_a_new_limit_applies_at_the_next_commit) {
    MemoryStorageEngine engine(5);
    for (int n = 1; n <= 3; ++n) {
        engine.commit(transaction(n));
    }
    engine.setStepLimit(2);
    BOOST_CHECK_EQUAL(engine.stepLimit(), 2);
    checkSteps(engine, 0, 3, 3);

    engine.commit(transaction(4));
    checkSteps(engine, 2, 4, 4);
}

// Eviction applies to executed steps only. Undone steps are truncated by the commit before the
// limit is applied, so the limit never discards a step that redo could reach.
BOOST_AUTO_TEST_CASE(test_eviction_after_truncation_counts_only_executed_steps) {
    MemoryStorageEngine engine(3);
    for (int n = 1; n <= 3; ++n) {
        engine.commit(transaction(n));
    }
    engine.stepBackward();
    engine.stepBackward();

    engine.commit(transaction(4));
    checkSteps(engine, 0, 2, 2);
}

BOOST_AUTO_TEST_CASE(test_reset_restarts_the_step_numbers) {
    MemoryStorageEngine engine(2);
    for (int n = 1; n <= 5; ++n) {
        engine.commit(transaction(n));
    }
    engine.reset();
    checkSteps(engine, 0, 0, 0);
    BOOST_CHECK(!engine.stepBackward());
}

BOOST_AUTO_TEST_SUITE_END()
