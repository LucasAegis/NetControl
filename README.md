# NetControl: A "Faraday Cage" for Your Windows Apps 🛡️

**Born from extreme paranoia. Built for absolute privacy.**

## 📖 The Story
I am a communication systems architect. In 2024, while drafting a 70,000-word top-secret patent, I realized that keyboards and background services are constantly "phoning home." To protect my intellectual property, I built the **Aegis Offline AI Keyboard** for mobile.

But my PC remained a battlefield of "leaky" apps. I tested every network blocker on the market—they were all bloated, complex, or felt like they were snooping themselves. 

**So, I built this: NetControl.** A tiny (4MB), 100% offline, portable network control center. It’s the "Physical Toggle" Windows should have had.

---

## ✨ Features
* **100% Portable:** No installation. No registry bloat. Just a 4MB `.exe`.
* **Zero Network Code:** This tool itself has no permission to access the internet. It is a "Faraday Cage" for your other apps.
* **Dead Simple UI:** * 🟢 **Green:** Allowed to "talk" to the world.
    * 🔴 **Red:** Total silence. Blocked apps drop to the bottom instantly.
* **UAC Native:** Double-click, hit 'Yes', and you own your network again.

---

## 📸 Interface
![Software Screenshot](https://github.com/LucasAegis/NetControl/raw/master/res/image_be63df.png)
*Visual feedback: Red means blocked, Green means allowed. Simple as that.*

### 🛡️ Real-World Example (The "Silence" List)
Here is a look at how I manage "leaky" background services. Once blocked, they are sent to the bottom of the list and stripped of all networking privileges:

![Blocked Apps Example](https://github.com/LucasAegis/NetControl/raw/master/res/blocked_apps_example.png)
*Visual proof: Common OEM services and system hosts are successfully isolated.*

---

## 🚀 How to Use
1. **Download:** Get `NetControl_v1.0.0_Portable.zip` from the [Releases](https://github.com/LucasAegis/NetControl/releases) page.
2. **Regarding Security Flags (False Positive):** * As an unsigned tool that modifies system firewall rules, Windows Defender may flag it as `Trojan:Win32/Wacatac.B!ml`.
   * **Latest Status:** A False Positive report has been officially submitted to Microsoft (Submission ID: `f1e70c43-2d84-41e4-bcda-b52dd98c1872`).
   * **Immediate Workaround:** To run it now, please add the file or its download folder to your **Windows Security Exclusions**, or manually select **"Allow on device"** in your protection history.
   * **Review:** I have open-sourced 100% of the code. Please **review the source code yourself** to verify its integrity.
3. **Run:** Double-click and grant Administrator privileges.
4. **Target & Kill:** Simply find the app you want to silence, click the **Red Button**, and it will stay blocked at the bottom of the list.

---

## ⚖️ License & Future Collaboration
This project is licensed under the **MIT License**.

**A Note from the Author:**
I built this tool purely for fun and to solve my own privacy needs. At this stage, **I no longer have the bandwidth to polish or maintain it further.**

However, I am a firm believer in the open-source spirit. 
* **If you are interested** in improving this tool;
* **And if you are willing** to keep your improved version open-source as well;

**Please feel free to reach out to me.** If your improvements are solid, I am happy to review and merge your code to update this repository using your enhanced version.

---
**Built for privacy. Powered by the community.**
