function runBleDataAcquisitionPlot_robust()
    % 适用于 Windows + MATLAB R2020b 及以上的 BLE 实时采集与绘图示例
    % 更新要点：
    % 1) 自动搜索名为 "TOGENASHI_TOGEARI" 的设备，并提取其 Address
    % 2) 用 Address 而非名称连接
    % 3) 若首次连接失败，会自动重试连接多次
    % 4) 成功后订阅 5 条特征并实时绘图、写入 CSV
    
    clc; clear; close all;

    global fileID
    global rollData pitchData yawData hrData spo2Data
    global rollLine pitchLine yawLine hrLine spo2Line
    global runFlag

    fileID = [];
    rollData = []; pitchData = []; yawData = []; hrData = []; spo2Data = [];
    runFlag = true;

    %%%%%%%%%%%%%%%% 0. 打开CSV文件写表头 %%%%%%%%%%%%%%%%
    csvFileName = "sensor_data_realtime.csv";
    fileID = fopen(csvFileName, 'w');
    fprintf(fileID, "Timestamp,Characteristic,Value\n");

    %%%%%%%%%%%%%%%% 1. 扫描并找目标设备 %%%%%%%%%%%%%%%%
    targetName = "TOGENASHI_TOGEARI";
    disp("Scanning for BLE devices... (this may take a few seconds)");
    devs = blelist;  % Windows/macOS 下可用
    disp(devs);

    % 根据名称匹配找索引
    idx = find(strcmp(devs.Name, targetName));
    if isempty(idx)
        % 在扫描结果里没搜到此名字
        warning("Did not find device '%s' by name in blelist. Trying partial match or manual address...", targetName);
        % 如果您知道确切地址，可直接写： deviceAddr = "2E1EE9A6C26D"; return;
        % 或者可以在 devs 中手动查看与“TOGENASHI_TOGEARI”最接近的
        % 这里先结束；实际可做更多逻辑
        fclose(fileID);
        error("Device named '%s' not found in scan results.", targetName);
    else
        % 如果同名设备有多个，取第一个
        idx = idx(1);
    end

    deviceAddr = devs.Address(idx);
    disp("Found device: " + targetName + ", Address = " + deviceAddr);

    %%%%%%%%%%%%%%%% 2. 多次尝试连接 %%%%%%%%%%%%%%%%
    b = tryConnectAddress(deviceAddr, 3);  % 尝试最多 3 次，若成功返回 ble 对象
    
    if isempty(b) || ~isvalid(b)
        fclose(fileID);
        error("Failed to connect to %s (Address=%s) after multiple tries.", targetName, deviceAddr);
    end
    
    disp("Successfully connected to " + b.Name + " with address " + deviceAddr);

    %%%%%%%%%%%%%%%% 3. 获取 Service/Characteristic UUID %%%%%%%%%%%%%%%%
    serviceUUID      = "19B10000-E8F2-537E-4F6C-D104768A1214";
    rollCharUUID     = "19B10001-E8F2-537E-4F6C-D104768A1214";
    pitchCharUUID    = "19B10002-E8F2-537E-4F6C-D104768A1214";
    yawCharUUID      = "19B10003-E8F2-537E-4F6C-D104768A1214";
    heartRateCharUUID= "19B10004-E8F2-537E-4F6C-D104768A1214";
    spo2CharUUID     = "19B10005-E8F2-537E-4F6C-D104768A1214";

    %%%%%%%%%%%%%%%% 4. 分别获取特征，并创建绘图 %%%%%%%%%%%%%%%%
    rollChar      = characteristic(b, serviceUUID, rollCharUUID);
    pitchChar     = characteristic(b, serviceUUID, pitchCharUUID);
    yawChar       = characteristic(b, serviceUUID, yawCharUUID);
    heartRateChar = characteristic(b, serviceUUID, heartRateCharUUID);
    spo2Char      = characteristic(b, serviceUUID, spo2CharUUID);

    % 绘图窗口
    f = figure('Name','BLE Real-Time Plot','NumberTitle','off');
    f.Position = [100 100 1200 600];
    tiledlayout(f, 2, 3, 'TileSpacing','compact','Padding','compact');

    % Roll
    ax1 = nexttile;
    rollLine = plot(ax1, NaN, NaN, '-b','LineWidth',1);
    title(ax1, 'Roll'); xlabel(ax1,'Time (s)'); ylabel(ax1,'Value');
    grid(ax1,'on'); hold(ax1,'on');

    % Pitch
    ax2 = nexttile;
    pitchLine = plot(ax2, NaN, NaN, '-r','LineWidth',1);
    title(ax2, 'Pitch'); xlabel(ax2,'Time (s)'); ylabel(ax2,'Value');
    grid(ax2,'on'); hold(ax2,'on');

    % Yaw
    ax3 = nexttile;
    yawLine = plot(ax3, NaN, NaN, '-g','LineWidth',1);
    title(ax3, 'Yaw'); xlabel(ax3,'Time (s)'); ylabel(ax3,'Value');
    grid(ax3,'on'); hold(ax3,'on');

    % HeartRate
    ax4 = nexttile;
    hrLine = plot(ax4, NaN, NaN, '-m','LineWidth',1);
    title(ax4, 'HeartRate'); xlabel(ax4,'Time (s)'); ylabel(ax4,'Value');
    grid(ax4,'on'); hold(ax4,'on');

    % SpO2
    ax5 = nexttile;
    spo2Line = plot(ax5, NaN, NaN, '-c','LineWidth',1);
    title(ax5, 'SpO2'); xlabel(ax5,'Time (s)'); ylabel(ax5,'Value');
    grid(ax5,'on'); hold(ax5,'on');

    %%%%%%%%%%%%%%%% 5. 订阅通知 %%%%%%%%%%%%%%%%
    subscribe(rollChar,      "notification", @(src,evt) bleCallback(src, evt, "Roll"));
    subscribe(pitchChar,     "notification", @(src,evt) bleCallback(src, evt, "Pitch"));
    subscribe(yawChar,       "notification", @(src,evt) bleCallback(src, evt, "Yaw"));
    subscribe(heartRateChar, "notification", @(src,evt) bleCallback(src, evt, "HeartRate"));
    subscribe(spo2Char,      "notification", @(src,evt) bleCallback(src, evt, "SpO2"));

    disp("All notifications subscribed. Close figure or call stopBlePlotAcquisition() to stop.");

    %%%%%%%%%%%%%%%% 6. 主循环 %%%%%%%%%%%%%%%%
    while isvalid(f) && runFlag
        pause(0.1);
        drawnow limitrate
    end

    %%%%%%%%%%%%%%%% 7. 退出清理 %%%%%%%%%%%%%%%%
    unsubscribe(rollChar);
    unsubscribe(pitchChar);
    unsubscribe(yawChar);
    unsubscribe(heartRateChar);
    unsubscribe(spo2Char);
    fclose(fileID);
    fileID = [];
    disp("Unsubscribed all notifications and closed CSV file.");
end


% ------------------------------------------------------------------------------
% 尝试用指定地址连接 BLE，多次重试
% ------------------------------------------------------------------------------
function bObj = tryConnectAddress(deviceAddr, maxAttempts)
    bObj = [];
    for attempt = 1:maxAttempts
        disp("Attempt #" + attempt + " connecting to: " + deviceAddr);
        try
            bObj = ble(deviceAddr);
            disp("Connection attempt succeeded.");
            return;  % 成功则退出函数
        catch ME
            warning("Connection attempt #%d failed: %s", attempt, ME.message);
            pause(2); % 等待2秒后重试
        end
    end
    % 如果到这里还没成功，就返回空
end


% ------------------------------------------------------------------------------
% 通知回调
% ------------------------------------------------------------------------------
function bleCallback(src, event, characteristicName)
    global fileID
    global rollData pitchData yawData hrData spo2Data
    global rollLine pitchLine yawLine hrLine spo2Line

    rawData = event.Data; 
    if length(rawData) == 4
        value = typecast(uint8(rawData), 'single'); 
    else
        value = NaN;
    end

    % 获取当前时间戳
    t = now;  
    tSec = (t - floor(t)) * 86400; % 当日秒数(仅演示)
    % 写 CSV
    fprintf(fileID, "%s,%s,%.4f\n", datestr(t,'yyyy-mm-dd HH:MM:SS'), characteristicName, value);

    % 更新绘图
    switch characteristicName
        case "Roll"
            rollData = [rollData; tSec, value];
            set(rollLine,'XData',rollData(:,1),'YData',rollData(:,2));
        case "Pitch"
            pitchData = [pitchData; tSec, value];
            set(pitchLine,'XData',pitchData(:,1),'YData',pitchData(:,2));
        case "Yaw"
            yawData = [yawData; tSec, value];
            set(yawLine,'XData',yawData(:,1),'YData',yawData(:,2));
        case "HeartRate"
            hrData = [hrData; tSec, value];
            set(hrLine,'XData',hrData(:,1),'YData',hrData(:,2));
        case "SpO2"
            spo2Data = [spo2Data; tSec, value];
            set(spo2Line,'XData',spo2Data(:,1),'YData',spo2Data(:,2));
        otherwise
            % 忽略未知特征
    end
end


% ------------------------------------------------------------------------------
% 结束函数：命令行执行 stopBlePlotAcquisition 主动停止
% ------------------------------------------------------------------------------
function stopBlePlotAcquisition()
    global runFlag
    runFlag = false;
    disp("Stop signal set. Wait for main loop to exit...");
end
