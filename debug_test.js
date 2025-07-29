// Simple test case to debug analyzer
export class ProjectCore {
    constructor() {
        this.plugins = [];
    }
    
    async createPlugin(name) {
        return {name: name};
    }
    
    static getInstance() {
        return new ProjectCore();
    }
}

function regularFunction() {
    return "hello";
}

const arrowFunc = () => "arrow";

async function asyncMain() {
    const core = new ProjectCore();
    const plugin = await core.createPlugin("test");
    return plugin;
}