# 1131-EOS Lab&HW

## Hw1｜外送管理系統（單機版，RPi）

- 單人點餐流程的模擬，含 3 家店、各 2 道餐點，操作包含 shop list、order（含 confirm / cancel）。
- **功能重點：** CLI 互動式選單、狀態管理（選店、選品、數量、確認／取消）、以 C + Makefile 結構化程式專案。

## Hw2｜外送平台（多人連線版：單一外送員、一次一位客戶）

- 把 Hw1 改成 socket server 版；同時間只服務一位客戶，支援 shop list、order 等協定互動（助教提供 client 檢測）。
- **功能重點：** TCP socket 程式設計（Server 端）、封包格式與指令解析、阻塞式 I/O、正確回覆訊息（結尾換行等）。

## Hw3｜外送平台（多人連線版：兩位外送員、可同時多客戶）

- 採 process 或 thread 改成多客戶 Concurrent，兩位外送員、FCFS 排隊，須避免 race condition；沿用 Hw2 指令並新增長等待的互動。
- **功能重點：** 多程序／多執行緒 Concurrent、同步與共享狀態一致性、排程（FCFS）。

## Lab 3-1｜學號跑馬燈（RPi）

- 撰寫 kernel driver + user-space writer，在硬體上做學號的跑馬燈顯示（可控制方向與速度）。
- **功能重點：** Linux kernel module、`/dev` 字元裝置、基本 I/O 控制。

## Lab 3-2｜學號跑馬燈－七段顯示器（RPi）

- 以 GPIO 驅動 7-seg，實作 driver + writer 呈現學號跑馬燈。
- **功能重點：** GPIO 腳位控制、七段碼（segment／位掃）、ISR／時間節拍與顯示時序。

## Lab 4｜簡易名字跑馬燈（VM）

- 在 VM 的終端機以 ASCII／16-seg 方式捲動顯示名字，只需 user-space 程式。
- **功能重點：** 字型切片／映射、字串緩衝與位移動畫、輸出排版。

## Lab 5｜東方快車

- Server／Client 範例，伺服器以定時／訊號機制推動列車動畫，並提供 `demo.sh` 檢查與展示。
- **功能重點：** Socket programming、signals 與 timer、檔案描述元操作（如 `dup2`）、子行程管理／避免 Zombie process。

## Lab 6｜Web ATM

- ATM 伺服器，支援多連線存提款／查詢等操作，需處理競態、維持一致性，提供 client 命令格式。
- **功能重點：** Concurrent 處理（多客戶）、臨界區／一致性控制、通訊協定與錯誤處理。

## Lab 7｜終極密碼（猜數字對戰）

- 兩支程式（game／guess）透過 shared memory 與 signal／timer 互動，根據提示範圍縮小直到猜中。
- **功能重點：** IPC（Shared Memory、FIFO）、訊號與計時器、程序間同步與資源清理。
