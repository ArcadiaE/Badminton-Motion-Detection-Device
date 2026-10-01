function runBleContinuousCaptureColumns()

    clc; clear; close all;

    csvFileName = "ble_columns_data.csv";
    fileID = fopen(csvFileName, "w");
    fprintf(fileID, "roll,pitch,yaw,heartRate,spo2\n");

    targetName = "TOGENASHI_TOGEARI";
    disp("Scanning for BLE devices...");
    devs = blelist;  % 扫描周边
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

    serviceUUID  = "19B10000-E8F2-537E-4F6C-D104768A1214";
    rollCharUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
    pitchCharUUID= "19B10002-E8F2-537E-4F6C-D104768A1214";
    yawCharUUID  = "19B10003-E8F2-537E-4F6C-D104768A1214";
    hrCharUUID   = "19B10004-E8F2-537E-4F6C-D104768A1214";
    spo2CharUUID = "19B10005-E8F2-537E-4F6C-D104768A1214";

    rollChar  = characteristic(b, serviceUUID, rollCharUUID);
    pitchChar = characteristic(b, serviceUUID, pitchCharUUID);
    yawChar   = characteristic(b, serviceUUID, yawCharUUID);
    hrChar    = characteristic(b, serviceUUID, hrCharUUID);
    spo2Char  = characteristic(b, serviceUUID, spo2CharUUID);

    disp("Start continuous capture (column format). Press Ctrl+C to stop...");

    while true
        rollVal  = readFloat(rollChar);
        pitchVal = readFloat(pitchChar);
        yawVal   = readFloat(yawChar);
        hrVal    = readFloat(hrChar);
        spo2Val  = readFloat(spo2Char);

        fprintf(fileID, "%.4f,%.4f,%.4f,%.4f,%.4f\n", ...
            rollVal, pitchVal, yawVal, hrVal, spo2Val);

        pause(0.5); % 间隔
    end
    
    % fclose(fileID);
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
