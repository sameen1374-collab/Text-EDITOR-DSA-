#include "AIManager.h"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// Use the official Gemini Interactions API endpoint and gemini-3.8-flash model
const std::string AIManager::API_ENDPOINT = "https://generativelanguage.googleapis.com/v1beta/interactions";
const std::string AIManager::MODEL_NAME = "gemini-3.6-flash";

// Check if the GEMINI_API_KEY environment variable is defined and non-empty
bool AIManager::isKeyConfigured() {
    const char* key = std::getenv("GEMINI_API_KEY");
    return (key != nullptr && std::string(key).length() > 0);
}

// Callback invoked by libcurl as response body data arrives
size_t AIManager::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    std::string* responseBuffer = static_cast<std::string*>(userp);
    responseBuffer->append(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

// Callback invoked by libcurl as response headers arrive
static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalBytes = size * nitems;
    std::string* headersBuffer = static_cast<std::string*>(userdata);
    headersBuffer->append(buffer, totalBytes);
    return totalBytes;
}

// Helper to inspect response headers for a "Retry-After" directive (in seconds)
static int getRetryAfterMs(const std::string& headers) {
    std::string lowerHeaders = headers;
    for (char& c : lowerHeaders) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    size_t pos = lowerHeaders.find("retry-after:");
    if (pos != std::string::npos) {
        size_t start = pos + 12;
        while (start < lowerHeaders.size() && (lowerHeaders[start] == ' ' || lowerHeaders[start] == '\t')) {
            start++;
        }
        size_t end = lowerHeaders.find_first_of("\r\n", start);
        std::string valStr = lowerHeaders.substr(start, end - start);
        try {
            int seconds = std::stoi(valStr);
            if (seconds > 0 && seconds <= 60) {
                return seconds * 1000; // convert seconds to milliseconds
            }
        } catch (...) {
            // Ignore non-integer date formats and fall back to exponential backoff
        }
    }
    return 0; // Not provided
}

// Low-level HTTPS POST handler with diagnostic retry feedback, Retry-After handling, and detailed error preservation
AIResult AIManager::sendRequest(const std::string& fullPrompt) {
    // 1. Securely check for the API key from the environment
    const char* apiKeyEnv = std::getenv("GEMINI_API_KEY");
    if (!apiKeyEnv || std::string(apiKeyEnv).empty()) {
        return {
            false,
            "",
            "API key not found. Please set the GEMINI_API_KEY environment variable in your terminal:\n"
            "  $env:GEMINI_API_KEY=\"your_key_here\""
        };
    }
    std::string apiKey = apiKeyEnv;

    // 2. Build the JSON request body
    nlohmann::json requestPayload;
    requestPayload["model"] = MODEL_NAME;
    requestPayload["input"] = fullPrompt;
    std::string requestBody = requestPayload.dump();

    // 3. Retry configuration
    const int maxRetries = 3;
    const int initialBackoffMs = 1500; // 1.5 seconds base
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> jitterDist(200, 700); // 200ms - 700ms random jitter

    std::string lastError = "";

    for (int attempt = 0; attempt <= maxRetries; attempt++) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return { false, "", "Failed to initialize network system (libcurl initialization error)." };
        }

        // Configure HTTP headers (authentication and content-type)
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string authHeader = "x-goog-api-key: " + apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());

        std::string responseBuffer;
        std::string headersBuffer;

        curl_easy_setopt(curl, CURLOPT_URL, API_ENDPOINT.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headersBuffer);

        // Security: SSL/TLS certificate and hostname verification enabled
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Extended timeouts for complex prompts under peak load
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        // Perform HTTPS request
        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        // Case A: Network or timeout failure
        if (res != CURLE_OK) {
            std::string curlError = curl_easy_strerror(res);
            lastError = "Network connection failed (" + curlError + ").";

            // If it's a timeout or transient network issue, retry with backoff
            if ((res == CURLE_OPERATION_TIMEDOUT || res == CURLE_COULDNT_CONNECT || res == CURLE_COULDNT_RESOLVE_HOST)
                && attempt < maxRetries) {
                int waitMs = (initialBackoffMs * (1 << attempt)) + jitterDist(gen);
                std::cout << "  [Retry " << (attempt + 1) << "/" << maxRetries
                          << "] Network issue: " << curlError << ". Retrying in "
                          << (waitMs / 1000.0) << "s...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
                continue;
            }
            return { false, "", lastError + " Please check your internet connection." };
        }

        // Case B: Success (HTTP 200)
        if (httpCode == 200) {
            nlohmann::json responseJson;
            try {
                responseJson = nlohmann::json::parse(responseBuffer);
            } catch (const nlohmann::json::parse_error&) {
                return { false, "", "Failed to parse AI response as valid JSON." };
            }

            // Extract generated text from Gemini Interactions API response structure
            std::string generatedText = "";
            if (responseJson.contains("steps") && responseJson["steps"].is_array()) {
                for (const auto& step : responseJson["steps"]) {
                    if (step.contains("type") && step["type"] == "model_output") {
                        if (step.contains("content") && step["content"].is_array()) {
                            for (const auto& part : step["content"]) {
                                if (part.contains("type") && part["type"] == "text" && part.contains("text")) {
                                    generatedText += part["text"].get<std::string>();
                                }
                            }
                        }
                    }
                }
            }

            // Fallback: If returned in standard generateContent format ("candidates")
            if (generatedText.empty() && responseJson.contains("candidates") && responseJson["candidates"].is_array() && !responseJson["candidates"].empty()) {
                const auto& cand = responseJson["candidates"][0];
                if (cand.contains("content") && cand["content"].contains("parts") && cand["content"]["parts"].is_array()) {
                    for (const auto& part : cand["content"]["parts"]) {
                        if (part.contains("text")) {
                            generatedText += part["text"].get<std::string>();
                        }
                    }
                }
            }

            if (generatedText.empty()) {
                if (responseJson.contains("error")) {
                    return { false, "", "API returned error: " + responseJson["error"].dump() };
                }
                return { false, "", "Textify AI returned an empty response." };
            }

            return { true, generatedText, "" };
        }

        // Case C: Non-200 HTTP response
        // Parse Google's exact structured error message without redaction of diagnostic details
        std::string detailedMsg = "";
        try {
            nlohmann::json errJson = nlohmann::json::parse(responseBuffer);
            if (errJson.contains("error") && errJson["error"].is_object()) {
                const auto& errObj = errJson["error"];
                if (errObj.contains("message") && errObj["message"].is_string()) {
                    detailedMsg = errObj["message"].get<std::string>();
                }
                if (errObj.contains("status") && errObj["status"].is_string()) {
                    std::string statusStr = errObj["status"].get<std::string>();
                    if (!detailedMsg.empty()) {
                        detailedMsg = "[" + statusStr + "] " + detailedMsg;
                    } else {
                        detailedMsg = "[" + statusStr + "]";
                    }
                }
                // Extract structured quota violations if present
                if (errObj.contains("details") && errObj["details"].is_array()) {
                    for (const auto& d : errObj["details"]) {
                        if (d.contains("violations") && d["violations"].is_array()) {
                            for (const auto& v : d["violations"]) {
                                if (v.contains("description") && v["description"].is_string()) {
                                    detailedMsg += " (" + v["description"].get<std::string>() + ")";
                                }
                            }
                        }
                    }
                }
            } else if (errJson.contains("message") && errJson["message"].is_string()) {
                detailedMsg = errJson["message"].get<std::string>();
            }
        } catch (...) {
            detailedMsg = responseBuffer;
        }

        // Check if error is transient HTTP 503 or 5xx (Service Unavailable / High Demand)
        bool is5xx = (httpCode == 503 || (httpCode >= 500 && httpCode <= 599));

        if (is5xx && attempt < maxRetries) {
            int retryAfterMs = getRetryAfterMs(headersBuffer);
            int waitMs = (retryAfterMs > 0) ? retryAfterMs : ((initialBackoffMs * (1 << attempt)) + jitterDist(gen));
            std::cout << "  [Retry " << (attempt + 1) << "/" << maxRetries
                      << "] Server returned HTTP " << httpCode << ". Retrying in "
                      << (waitMs / 1000.0) << "s...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
            continue;
        }

        // Handling HTTP 429 (Rate Limit / Quota Exhaustion)
        if (httpCode == 429) {
            int retryAfterMs = getRetryAfterMs(headersBuffer);
            // Only retry 429 if the server specifically gave a Retry-After header <= 30 seconds
            if (retryAfterMs > 0 && retryAfterMs <= 30000 && attempt < 1) {
                std::cout << "  [Retry 1/1] HTTP 429 rate limit. Server requested wait of "
                          << (retryAfterMs / 1000.0) << "s (Retry-After). Waiting...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(retryAfterMs));
                continue;
            }
            // If no Retry-After or quota is exhausted, fail fast and report Google's actual detailed reason
            return {
                false,
                "",
                "Quota / Rate Limit Exceeded (HTTP 429): " + (detailedMsg.empty() ? "Request rate limit reached." : detailedMsg)
            };
        }

        // Specific HTTP status error formatting with preserved detailed message
        if (httpCode == 400) {
            return { false, "", "Bad Request (HTTP 400): " + (detailedMsg.empty() ? "Invalid prompt format." : detailedMsg) };
        } else if (httpCode == 401 || httpCode == 403) {
            return { false, "", "Authentication Failed (HTTP " + std::to_string(httpCode) + "): " + (detailedMsg.empty() ? "Check your GEMINI_API_KEY." : detailedMsg) };
        } else if (httpCode == 503) {
            return { false, "", "Service Unavailable (HTTP 503): " + (detailedMsg.empty() ? "The model is currently experiencing high demand. Please try again shortly." : detailedMsg) };
        } else if (httpCode >= 500) {
            return { false, "", "Server Error (HTTP " + std::to_string(httpCode) + "): " + (detailedMsg.empty() ? "Service temporarily unavailable." : detailedMsg) };
        }

        return { false, "", "HTTP Error " + std::to_string(httpCode) + (detailedMsg.empty() ? "" : ": " + detailedMsg) };
    }

    return { false, "", "Request failed after maximum retries. " + lastError };
}

// Core prompt handler: formats document context if provided
AIResult AIManager::generate(const std::string& prompt, const std::string& context) {
    if (prompt.empty() && context.empty()) {
        return { false, "", "Cannot send an empty request to AI." };
    }

    std::string fullPrompt;
    if (!context.empty()) {
        fullPrompt = "You are Textify AI, an intelligent assistant inside a C++ text editor.\n"
                     "Below is the relevant text from the user's document:\n"
                     "\"\"\"\n" + context + "\n\"\"\"\n\n"
                     "User Request: " + prompt;
    } else {
        fullPrompt = prompt;
    }

    return sendRequest(fullPrompt);
}

// 1. Ask Textify AI
AIResult AIManager::ask(const std::string& question, const std::string& docContext) {
    return generate(question, docContext);
}

// Helper to check if a string is empty or contains only whitespace characters
static bool isBlank(const std::string& s) {
    return s.empty() || s.find_first_not_of(" \t\n\r\f\v") == std::string::npos;
}

// 2. Summarize
AIResult AIManager::summarize(const std::string& textToSummarize) {
    if (isBlank(textToSummarize)) {
        return { false, "", "Input text cannot be empty." };
    }
    std::string prompt = "Provide a concise, clear, and well-structured summary of the following text:\n\n" + textToSummarize;
    return sendRequest(prompt);
}

// 3. Explain
AIResult AIManager::explain(const std::string& textToExplain) {
    if (isBlank(textToExplain)) {
        return { false, "", "Input text cannot be empty." };
    }
    std::string prompt = "Explain the concepts and meaning in the following text in simple, easy-to-understand language:\n\n" + textToExplain;
    return sendRequest(prompt);
}

// 4. Generate Key Points
AIResult AIManager::generateKeyPoints(const std::string& text) {
    if (isBlank(text)) {
        return { false, "", "Input text cannot be empty." };
    }
    std::string prompt = "Extract the key points from the following text and format them as a bulleted list:\n\n" + text;
    return sendRequest(prompt);
}

// 5. Grammar Check
AIResult AIManager::checkGrammar(const std::string& textToCheck) {
    if (isBlank(textToCheck)) {
        return { false, "", "Input text cannot be empty." };
    }
    std::string prompt = "Review the following text for grammar, spelling, punctuation, and phrasing mistakes. "
                         "Explain the errors found, provide suggestions, and output the corrected version:\n\n" + textToCheck;
    return sendRequest(prompt);
}

// 6. Improve Writing
AIResult AIManager::improveWriting(const std::string& textToImprove) {
    if (isBlank(textToImprove)) {
        return { false, "", "Input text cannot be empty." };
    }
    std::string prompt = "Enhance the vocabulary, clarity, tone, and sentence flow of the following text while preserving its original meaning:\n\n" + textToImprove;
    return sendRequest(prompt);
}
