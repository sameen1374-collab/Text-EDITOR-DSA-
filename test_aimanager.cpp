#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "AIManager.h"

// Helper function to print section dividers
void printHeader(const std::string& title) {
    std::cout << "\n=======================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "=======================================================\n";
}

// Pacing delay between live test cases to respect API limits
void paceRequest() {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

// Sample text fixtures
const std::string sampleDoc =
    "A Doubly Linked List is a linear data structure where each element (called a node)\n"
    "contains three fields: two link references (one to the previous node and one to the next node)\n"
    "and one data field. This enables bidirectional traversal through the sequence.\n"
    "Insertion and deletion at known positions run in O(1) time complexity.";

const std::string sampleRoughText =
    "They is going to the lab yesturday and forgot they're notebook. "
    "The code was write by a student who not know C++ good.";

void runAskTest() {
    printHeader("LIVE TEST: Feature 1 - Ask Textify AI");
    std::cout << "Prompt: 'What is the time complexity of searching in a Doubly Linked List?'\n";
    AIResult res = AIManager::ask("What is the time complexity of searching in a Doubly Linked List?", sampleDoc);
    if (res.success) {
        std::cout << "[PASS] Response received:\n\n" << res.text << "\n";
    } else {
        std::cout << "[FAIL] Error reported:\n" << res.error << "\n";
    }
}

void runSummarizeTest() {
    printHeader("LIVE TEST: Feature 2 - Summarize");
    AIResult res = AIManager::summarize(sampleDoc);
    if (res.success) {
        std::cout << "[PASS] Response received:\n\n" << res.text << "\n";
    } else {
        std::cout << "[FAIL] Error reported:\n" << res.error << "\n";
    }
}

void runExplainTest() {
    printHeader("LIVE TEST: Feature 3 - Explain");
    AIResult res = AIManager::explain("Bidirectional traversal and pointer manipulation in doubly linked lists.");
    if (res.success) {
        std::cout << "[PASS] Response received:\n\n" << res.text << "\n";
    } else {
        std::cout << "[FAIL] Error reported:\n" << res.error << "\n";
    }
}

void runKeyPointsTest() {
    printHeader("LIVE TEST: Feature 4 - Generate Key Points");
    AIResult res = AIManager::generateKeyPoints(sampleDoc);
    if (res.success) {
        std::cout << "[PASS] Response received:\n\n" << res.text << "\n";
    } else {
        std::cout << "[FAIL] Error reported:\n" << res.error << "\n";
    }
}

void runGrammarTest() {
    printHeader("LIVE TEST: Feature 5 - Grammar Check");
    std::cout << "Input text with deliberate errors: \"" << sampleRoughText << "\"\n\n";
    AIResult res = AIManager::checkGrammar(sampleRoughText);
    if (res.success) {
        std::cout << "[PASS] Response received:\n\n" << res.text << "\n";
    } else {
        std::cout << "[FAIL] Error reported:\n" << res.error << "\n";
    }
}

void runImproveTest() {
    printHeader("LIVE TEST: Feature 6 - Improve Writing");
    std::cout << "Input text to enhance: \"" << sampleRoughText << "\"\n\n";
    AIResult res = AIManager::improveWriting(sampleRoughText);
    if (res.success) {
        std::cout << "[PASS] Response received:\n\n" << res.text << "\n";
    } else {
        std::cout << "[FAIL] Error reported:\n" << res.error << "\n";
    }
}

int main() {
    printHeader("TEXTIFY AI - COMPREHENSIVE PHASE 4 TEST HARNESS");

    // ---------------------------------------------------------
    // LOCAL TEST 1: Input Validation (Runs offline)
    // ---------------------------------------------------------
    printHeader("LOCAL TEST 1: Input Validation (Empty & Whitespace Prompts)");
    AIResult emptyTest = AIManager::generate("", "");
    if (!emptyTest.success && !emptyTest.error.empty()) {
        std::cout << "[PASS] Empty prompt correctly rejected by generate().\n";
        std::cout << "       Reason: " << emptyTest.error << "\n";
    } else {
        std::cout << "[FAIL] Empty prompt was not rejected by generate()!\n";
    }

    // Comprehensive offline empty & whitespace tests for all 5 specialized functions
    struct FeatureValidator {
        std::string name;
        AIResult (*func)(const std::string&);
    };

    FeatureValidator validators[] = {
        { "summarize", &AIManager::summarize },
        { "explain", &AIManager::explain },
        { "generateKeyPoints", &AIManager::generateKeyPoints },
        { "checkGrammar", &AIManager::checkGrammar },
        { "improveWriting", &AIManager::improveWriting }
    };

    bool allValidationPassed = true;
    for (const auto& f : validators) {
        // 1. Test empty string input ""
        AIResult resEmpty = f.func("");
        if (!resEmpty.success && resEmpty.error == "Input text cannot be empty.") {
            std::cout << "[PASS] " << f.name << "(\"\") correctly rejected with: \"" << resEmpty.error << "\"\n";
        } else {
            std::cout << "[FAIL] " << f.name << "(\"\") was not rejected properly! (error=\"" << resEmpty.error << "\")\n";
            allValidationPassed = false;
        }

        // 2. Test whitespace-only input "   \t\n\r  "
        AIResult resWhitespace = f.func("   \t\n\r  ");
        if (!resWhitespace.success && resWhitespace.error == "Input text cannot be empty.") {
            std::cout << "[PASS] " << f.name << "(\"  \\t\\n\\r \") correctly rejected with: \"" << resWhitespace.error << "\"\n";
        } else {
            std::cout << "[FAIL] " << f.name << "(\"  \\t\\n\\r \") was not rejected properly! (error=\"" << resWhitespace.error << "\")\n";
            allValidationPassed = false;
        }

        // 3. Confirm valid non-empty input passes input validation
        // (It proceeds beyond validation into network dispatch, and does NOT return "Input text cannot be empty.")
        AIResult resValid = f.func("Sample valid text fixture for offline validation");
        if (resValid.error != "Input text cannot be empty.") {
            std::cout << "[PASS] " << f.name << "(valid text) successfully passed empty-input check.\n";
        } else {
            std::cout << "[FAIL] " << f.name << "(valid text) was incorrectly rejected by empty-input check!\n";
            allValidationPassed = false;
        }
    }

    if (allValidationPassed) {
        std::cout << "\n>>> [ALL 5 FEATURES PASSED INPUT VALIDATION TESTS (EMPTY, WHITESPACE, & VALID)] <<<\n";
    } else {
        std::cout << "\n>>> [SOME INPUT VALIDATION TESTS FAILED!] <<<\n";
    }

    // ---------------------------------------------------------
    // LOCAL TEST 2: Environment & API Key Check (Runs offline)
    // ---------------------------------------------------------
    printHeader("LOCAL TEST 2: GEMINI_API_KEY Detection");
    bool hasKey = AIManager::isKeyConfigured();
    if (!hasKey) {
        std::cout << "[INFO] GEMINI_API_KEY is not currently set in this terminal session.\n";
        std::cout << "       Testing graceful degradation on missing key...\n";
        AIResult missingKeyRes = AIManager::ask("Hello");
        if (!missingKeyRes.success && missingKeyRes.error.find("API key not found") != std::string::npos) {
            std::cout << "[PASS] Missing API key handled cleanly without crash.\n";
            std::cout << "       Error message:\n" << missingKeyRes.error << "\n";
        } else {
            std::cout << "[FAIL] Missing key handling did not produce expected error.\n";
        }

        std::cout << "\n-------------------------------------------------------\n";
        std::cout << "NOTICE: To run live API tests for the 6 AI features:\n";
        std::cout << "1. In PowerShell, set your key:\n";
        std::cout << "     $env:GEMINI_API_KEY=\"your_key_here\"\n";
        std::cout << "2. Then run: .\\test_aimanager.exe\n";
        std::cout << "-------------------------------------------------------\n";
        return 0;
    }

    std::cout << "[PASS] GEMINI_API_KEY detected in environment!\n";

    // ---------------------------------------------------------
    // SELECTIVE LIVE FEATURE TESTING MENU
    // ---------------------------------------------------------
    std::cout << "\n=======================================================\n";
    std::cout << "  SELECTIVE LIVE FEATURE TESTING MENU\n";
    std::cout << "=======================================================\n";
    std::cout << "  1. Test Feature 1: Ask Textify AI\n";
    std::cout << "  2. Test Feature 2: Summarize\n";
    std::cout << "  3. Test Feature 3: Explain\n";
    std::cout << "  4. Test Feature 4: Generate Key Points\n";
    std::cout << "  5. Test Feature 5: Grammar Check\n";
    std::cout << "  6. Test Feature 6: Improve Writing\n";
    std::cout << "  7. Run ALL Six Live Features\n";
    std::cout << "  0. Skip Live Calls (Offline Validation Only)\n";
    std::cout << "Choose an option (0-7): ";

    std::string inputChoice;
    if (!std::getline(std::cin, inputChoice) || inputChoice.empty()) {
        std::cout << "\n[INFO] No option entered or automated run. Skipping live calls.\n";
        return 0;
    }

    int choice = -1;
    try {
        choice = std::stoi(inputChoice);
    } catch (...) {
        choice = -1;
    }

    switch (choice) {
    case 1:
        runAskTest();
        break;
    case 2:
        runSummarizeTest();
        break;
    case 3:
        runExplainTest();
        break;
    case 4:
        runKeyPointsTest();
        break;
    case 5:
        runGrammarTest();
        break;
    case 6:
        runImproveTest();
        break;
    case 7:
        runAskTest();
        paceRequest();
        runSummarizeTest();
        paceRequest();
        runExplainTest();
        paceRequest();
        runKeyPointsTest();
        paceRequest();
        runGrammarTest();
        paceRequest();
        runImproveTest();
        break;
    case 0:
        std::cout << "\n[INFO] Live API calls skipped. Offline tests completed.\n";
        break;
    default:
        std::cout << "\nInvalid choice. Skipping live API calls.\n";
        break;
    }

    printHeader("TEST HARNESS COMPLETED");
    return 0;
}
