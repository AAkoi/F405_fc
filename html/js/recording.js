// 数据录制模块
class RecordingManager {
    constructor() {
        this.isRecording = false;
        this.recordedData = [];
        this.recordStartTime = 0;
    }

    start() {
        this.isRecording = true;
        this.recordedData = [];
        this.recordStartTime = Date.now();
    }

    stop() {
        this.isRecording = false;
    }

    addDataPoint(gyroData) {
        if (!this.isRecording) return;

        this.recordedData.push({
            t: Date.now() - this.recordStartTime,
            gx: gyroData.x,
            gy: gyroData.y,
            gz: gyroData.z
        });
    }

    export() {
        if (this.recordedData.length === 0) {
            alert('没有录制到数据');
            return;
        }

        // 生成 CSV 内容（IMU_CSV 格式）
        let csvContent = '# IMU Recording - ' + new Date().toLocaleString() + '\n';
        csvContent += '# Format: IMU_CSV,t_ms,raw_gx,raw_gy,raw_gz,filt_gx,filt_gy,filt_gz\n';
        
        this.recordedData.forEach(row => {
            csvContent += `IMU_CSV,${row.t},${row.gx},${row.gy},${row.gz},${row.gx},${row.gy},${row.gz}\n`;
        });

        // 创建下载链接
        const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
        const link = document.createElement('a');
        const url = URL.createObjectURL(blob);
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5);
        
        link.setAttribute('href', url);
        link.setAttribute('download', `imu_recording_${timestamp}.csv`);
        link.style.visibility = 'hidden';
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        URL.revokeObjectURL(url);

        const duration = ((this.recordedData[this.recordedData.length-1].t - this.recordedData[0].t) / 1000).toFixed(2);
        alert(`录制完成！\n数据点: ${this.recordedData.length}\n时长: ${duration} 秒`);
    }

    getStatus() {
        return this.isRecording;
    }

    getDataCount() {
        return this.recordedData.length;
    }
}

