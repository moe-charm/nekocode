// Simple utility functions
export const helpers = {
    isEmpty: (value) => {
        return value === null || value === undefined || value === '';
    },
    
    async delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
};

class Logger {
    constructor(level = 'info') {
        this.level = level;
    }
    
    log(message) {
        console.log(`[${this.level}] ${message}`);
    }
}

export default Logger;