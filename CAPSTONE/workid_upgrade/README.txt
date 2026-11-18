WORKID - DIGITAL WORK IDENTITY SYSTEM (C++ PROTOTYPE)

FEATURES
- Administrator dashboard with worker totals and verification overview
- Worker registration with unique Worker IDs (DWI1001, DWI1002, ...)
- Worker directory with search, filters and CSV export
- Skill verification status management
- Unique QR code for every worker in the directory
- Owner portal to scan a worker QR code or enter the Worker ID manually
- Owner/supervisor feedback form with workplace, 1-5 rating and comments
- Owner feedback history linked to the worker's ID

RUN ON macOS
1. Extract this ZIP file.
2. Open the extracted folder in VS Code.
3. Open Terminal > New Terminal.
4. Compile:
   clang++ -std=c++17 server.cpp -o digital_identity_server
5. Run:
   ./digital_identity_server
6. Open Safari or Chrome:
   http://localhost:8080
7. Keep the Terminal window open while using the website. Press Control+C to stop.

QR SCANNING
- QR images are created from each worker's profile URL.
- The QR generator and camera scanner load from public CDN libraries, so internet access is required for those features.
- Camera access is supported on localhost. For a separate owner's phone to scan the QR, run the server on a computer and use that computer's local network IP address in the QR URL. Both devices must be on the same Wi-Fi network and the firewall must allow port 8080.

IMPORTANT PROTOTYPE LIMITATIONS
- Worker and feedback records are stored in memory and are cleared when the server stops.
- This is a demonstration prototype, not a production identity or employment verification service.
- Owner feedback is not authenticated. A production version needs owner accounts, passwords, role-based access, secure QR tokens, database storage (e.g. MySQL), HTTPS, audit logs, input validation and worker consent/privacy controls.
- A QR code identifies a worker profile; it does not itself prove the person's identity or the truth of a review.
