// テスト用JavaScriptファイル
function test() {
    return "hello";
}

class MyClass {
    constructor() {
        this.value = 42;
    }
    
    getValue() {
        return this.value;
    }
}

const arrow = () => {
    console.log("Arrow function");
};

// CommonJS
const fs = require('fs');

// ES6 import
import { something } from './module';

module.exports = MyClass;