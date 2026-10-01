function runBleContinuousCaptureColumnsPlot()
    clc; clear; close all;

    timestamp = datestr(now, 'yyyymmdd_HHMM');
    csvFileName = sprintf("pre_test_%s.csv", timestamp);
    
    fileID = fopen(csvFileName, 'w');
    if fileID == -1
        error('Fail to create/open file %s, please check the path and make sure file is not in use!', csvFileName);
    end

    fprintf(fileID, 'timestamp,roll,pitch,yaw,heartRate,spo2,roll_velocity,pitch_velocity,yaw_velocity,roll_acceleration,pitch_acceleration,yaw_acceleration,peak_count\n');

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

    disp("Start continuous capture (column format) + feature extraction. Press Ctrl+C to stop...");
    
    dt = 1/200;  % 采样率 200Hz

    timeData = [];
    rollData = [];
    pitchData = [];
    yawData = [];
    hrData = [];
    spo2Data = [];
    
    rollVelocityData = [];
    pitchVelocityData = [];
    yawVelocityData = [];
    
    rollAccelerationData = [];
    pitchAccelerationData = [];
    yawAccelerationData = [];
    
    peak_count = 0;
    
    t0 = tic;  

    % **初始化绘图**
    figure;
    subplot(2,3,1); rollPlot = animatedline('Color', 'b'); title('Roll'); grid on; hold on;
    subplot(2,3,2); pitchPlot = animatedline('Color', 'r'); title('Pitch'); grid on; hold on;
    subplot(2,3,3); yawPlot = animatedline('Color', 'g'); title('Yaw'); grid on; hold on;
    subplot(2,3,4); hrPlot = animatedline('Color', 'm'); title('Heart Rate'); grid on; hold on;
    subplot(2,3,5); spo2Plot = animatedline('Color', 'c'); title('SpO2'); grid on; hold on;

    while true
        elapsedSec = toc(t0);  

        rVal  = readFloat(rollChar);
        pVal  = readFloat(pitchChar);
        yVal  = readFloat(yawChar);
        hVal  = readFloat(hrChar);
        sVal  = readFloat(spo2Char);

        if ~isempty(rollData)
            roll_velocity = (rVal - rollData(end)) / dt;
            pitch_velocity = (pVal - pitchData(end)) / dt;
            yaw_velocity = (yVal - yawData(end)) / dt;
        else
            roll_velocity = 0;
            pitch_velocity = 0;
            yaw_velocity = 0;
        end
        
        rollVelocityData = [rollVelocityData, roll_velocity];
        pitchVelocityData = [pitchVelocityData, pitch_velocity];
        yawVelocityData = [yawVelocityData, yaw_velocity];

        if length(rollVelocityData) > 1
            roll_acceleration = (rollVelocityData(end) - rollVelocityData(end-1)) / dt;
            pitch_acceleration = (pitchVelocityData(end) - pitchVelocityData(end-1)) / dt;
            yaw_acceleration = (yawVelocityData(end) - yawVelocityData(end-1)) / dt;
        else
            roll_acceleration = 0;
            pitch_acceleration = 0;
            yaw_acceleration = 0;
        end

        rollAccelerationData = [rollAccelerationData, roll_acceleration];
        pitchAccelerationData = [pitchAccelerationData, pitch_acceleration];
        yawAccelerationData = [yawAccelerationData, yaw_acceleration];

        peak_count = countPeaks(rollVelocityData);

        fprintf(fileID, '%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d\n', ...
            elapsedSec, rVal, pVal, yVal, hVal, sVal, roll_velocity, pitch_velocity, yaw_velocity, roll_acceleration, pitch_acceleration, yaw_acceleration, peak_count);
        
        % **实时更新曲线**
        addpoints(rollPlot, elapsedSec, rVal);
        addpoints(pitchPlot, elapsedSec, pVal);
        addpoints(yawPlot, elapsedSec, yVal);
        addpoints(hrPlot, elapsedSec, hVal);
        addpoints(spo2Plot, elapsedSec, sVal);

        drawnow;
        pause(dt);
    end

    fclose(fileID);
end

function val = readFloat(charObj)
    rawData = read(charObj);
    if length(rawData) == 4
        val = typecast(uint8(rawData), 'single');  
    else
        val = NaN;  
    end
end

function peakCount = countPeaks(data)
    if length(data) < 3
        peakCount = 0;
        return;
    end
    peakCount = sum(diff(sign(diff(data))) < 0);
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
