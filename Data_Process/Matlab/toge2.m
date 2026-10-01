function runBleContinuousCaptureColumnsPlot()
    %{
    功能：
      1) 扫描并连接 "TOGENASHI_TOGEARI"
      2) 在 CSV 中按列存储：
         roll,pitch,yaw,heartRate,spo2
         1.2345,2.3456,3.4567,70.12,96.80
         ...
      3) 实时绘图：5 条曲线在同一窗口中不断更新
      4) 无限循环采集，Ctrl+C 停止
    %}

    clc; clear; close all;

    csvFileName = "ble_columns_data.csv";
    fileID = fopen(csvFileName, "w");
    fprintf(fileID, "roll,pitch,yaw,heartRate,spo2\n"); 

    targetName = "TOGENASHI_TOGEARI";
    disp("Scanning for BLE devices...");
    devs = blelist;  
    disp(devs);

    idx = find(strcmp(devs.Name, targetName), 1);
    if isempty(idx)
        fclose(fileID);
        error("Device '%s' not found in scan results.", targetName);
    end
    deviceAddr = devs.Address(idx);
    disp("Found device: " + targetName + ", Address = " + deviceAddr);

    b = tryConnectAddress(deviceAddr, 3);
    if isempty(b) || ~isvalid(b)
        fclose(fileID);
        error("Failed to connect to %s (%s).", targetName, deviceAddr);
    end
    disp("Connected to " + targetName + " (" + deviceAddr + ")");

    serviceUUID   = "19B10000-E8F2-537E-4F6C-D104768A1214";
    rollCharUUID  = "19B10001-E8F2-537E-4F6C-D104768A1214";
    pitchCharUUID = "19B10002-E8F2-537E-4F6C-D104768A1214";
    yawCharUUID   = "19B10003-E8F2-537E-4F6C-D104768A1214";
    hrCharUUID    = "19B10004-E8F2-537E-4F6C-D104768A1214";
    spo2CharUUID  = "19B10005-E8F2-537E-4F6C-D104768A1214";

    rollChar  = characteristic(b, serviceUUID, rollCharUUID);
    pitchChar = characteristic(b, serviceUUID, pitchCharUUID);
    yawChar   = characteristic(b, serviceUUID, yawCharUUID);
    hrChar    = characteristic(b, serviceUUID, hrCharUUID);
    spo2Char  = characteristic(b, serviceUUID, spo2CharUUID);

    f = figure('Name','BLE Real-Time Plot','NumberTitle','off');
    % 2×3 布局（留一个空格），可根据需要调整
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

    disp("Start continuous capture (column format) + real-time plot. Press Ctrl+C to stop...");

    timeData   = []; 
    rollData   = [];
    pitchData  = [];
    yawData    = [];
    hrData     = [];
    spo2Data   = [];

    t0 = tic;  

    while isvalid(f)
        elapsedSec = toc(t0); 

        rVal  = readFloat(rollChar);
        pVal  = readFloat(pitchChar);
        yVal  = readFloat(yawChar);
        hVal  = readFloat(hrChar);
        sVal  = readFloat(spo2Char);

        fprintf(fileID, "%.4f,%.4f,%.4f,%.4f,%.4f\n", ...
            rVal, pVal, yVal, hVal, sVal);

        timeData  = [timeData;  elapsedSec];
        rollData  = [rollData;  rVal];
        pitchData = [pitchData; pVal];
        yawData   = [yawData;   yVal];
        hrData    = [hrData;    hVal];
        spo2Data  = [spo2Data;  sVal];

        set(rollLine, 'XData', timeData, 'YData', rollData);
        set(pitchLine,'XData', timeData, 'YData', pitchData);
        set(yawLine,  'XData', timeData, 'YData', yawData);
        set(hrLine,   'XData', timeData, 'YData', hrData);
        set(spo2Line, 'XData', timeData, 'YData', spo2Data);

        drawnow limitrate;
        pause(0.01); 
    end

    fclose(fileID);
end

function bObj = tryConnectAddress(deviceAddr, maxTry)
    bObj = [];
    for attempt = 1:maxTry
        disp("Attempt #" + attempt + ": connecting to " + deviceAddr);
        try
            bObj = ble(deviceAddr);
            disp("Connection attempt succeeded.");
            return;
        catch ME
            warning("Attempt #%d failed: %s", attempt, ME.message);
            pause(2);
        end
    end
end

function val = readFloat(charObj)
    rawData = read(charObj);
    if length(rawData)==4
        val = typecast(uint8(rawData), 'single');
    else
        val = NaN;
    end
end
