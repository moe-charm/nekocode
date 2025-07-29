/**
 * 📜 TypeScript Complex Test Sample - interface・generics・export テスト
 */

// 🎯 インターフェース定義
interface User {
    id: number;
    name: string;
    email: string;
    metadata?: Record<string, any>;
}

interface Admin extends User {
    permissions: string[];
    level: number;
}

// 🔧 ジェネリックインターフェース
interface Repository<T> {
    find(id: number): Promise<T | null>;
    findAll(): Promise<T[]>;
    save(entity: T): Promise<T>;
    delete(id: number): Promise<boolean>;
}

interface CacheableRepository<T> extends Repository<T> {
    invalidateCache(): void;
    getCacheStats(): CacheStats;
}

interface CacheStats {
    hits: number;
    misses: number;
    size: number;
}

// 🎨 型エイリアスとユニオン型
type ID = string | number;
type Status = 'active' | 'inactive' | 'pending' | 'deleted';

type AsyncFunction<T> = () => Promise<T>;
type Callback<T> = (error: Error | null, result?: T) => void;

// 🏗️ ジェネリッククラス実装
export class UserRepository<U extends User> implements CacheableRepository<U> {
    private cache: Map<number, U> = new Map();
    private cacheStats: CacheStats = { hits: 0, misses: 0, size: 0 };

    constructor(private readonly dbConnection: string) {}

    async find(id: number): Promise<U | null> {
        // キャッシュチェック
        if (this.cache.has(id)) {
            this.cacheStats.hits++;
            return this.cache.get(id)!;
        }

        this.cacheStats.misses++;
        
        // 複雑な条件分岐で複雑度を上げる
        if (id < 0) {
            throw new Error('Invalid ID');
        } else if (id === 0) {
            return null;
        } else if (id < 100) {
            // モックデータ返却
            const user = this.createMockUser(id) as U;
            this.cache.set(id, user);
            return user;
        } else {
            // DBから取得するシミュレーション
            await this.simulateDbDelay();
            return null;
        }
    }

    async findAll(): Promise<U[]> {
        await this.simulateDbDelay();
        const users: U[] = [];
        
        // 複雑なループ処理
        for (let i = 1; i <= 10; i++) {
            if (i % 2 === 0) {
                users.push(this.createMockUser(i) as U);
            } else if (i % 3 === 0) {
                continue;
            } else {
                const user = this.createMockUser(i * 2) as U;
                users.push(user);
            }
        }
        
        return users;
    }

    async save(entity: U): Promise<U> {
        await this.simulateDbDelay();
        this.cache.set(entity.id, entity);
        this.cacheStats.size = this.cache.size;
        return entity;
    }

    async delete(id: number): Promise<boolean> {
        await this.simulateDbDelay();
        const existed = this.cache.has(id);
        this.cache.delete(id);
        this.cacheStats.size = this.cache.size;
        return existed;
    }

    invalidateCache(): void {
        this.cache.clear();
        this.cacheStats = { hits: 0, misses: 0, size: 0 };
    }

    getCacheStats(): CacheStats {
        return { ...this.cacheStats };
    }

    private createMockUser(id: number): User {
        return {
            id,
            name: `User ${id}`,
            email: `user${id}@example.com`,
            metadata: { created: new Date() }
        };
    }

    private async simulateDbDelay(): Promise<void> {
        await new Promise(resolve => setTimeout(resolve, 10));
    }
}

// 🌟 高度なジェネリック関数
export function pipe<T>(...fns: Array<(arg: T) => T>): (value: T) => T {
    return (value: T) => fns.reduce((acc, fn) => fn(acc), value);
}

export function curry<T extends (...args: any[]) => any>(
    fn: T
): T extends (arg: infer A) => infer R
    ? (arg: A) => R
    : T extends (arg1: infer A1, arg2: infer A2) => infer R
    ? (arg1: A1) => (arg2: A2) => R
    : T extends (arg1: infer A1, arg2: infer A2, arg3: infer A3) => infer R
    ? (arg1: A1) => (arg2: A2) => (arg3: A3) => R
    : never {
    return ((...args: any[]) => {
        if (args.length >= fn.length) {
            return fn(...args);
        }
        return (...nextArgs: any[]) => curry(fn)(...args, ...nextArgs);
    }) as any;
}

// 🎯 条件型とマップ型
type Nullable<T> = T | null | undefined;
type ReadonlyDeep<T> = {
    readonly [K in keyof T]: T[K] extends object ? ReadonlyDeep<T[K]> : T[K];
};

type PickByType<T, U> = {
    [K in keyof T as T[K] extends U ? K : never]: T[K];
};

// 🔄 非同期イテレータ
export class AsyncDataGenerator<T> implements AsyncIterable<T> {
    constructor(
        private generator: () => AsyncGenerator<T>,
        private maxItems: number = 100
    ) {}

    async *[Symbol.asyncIterator](): AsyncIterator<T> {
        let count = 0;
        const gen = this.generator();
        
        for await (const item of gen) {
            if (count >= this.maxItems) {
                break;
            }
            yield item;
            count++;
        }
    }
}

// 🎮 デコレータ（実験的機能）
export function LogExecution(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    const originalMethod = descriptor.value;
    
    descriptor.value = async function(...args: any[]) {
        console.log(`Executing ${propertyKey} with args:`, args);
        const start = Date.now();
        
        try {
            const result = await originalMethod.apply(this, args);
            console.log(`${propertyKey} completed in ${Date.now() - start}ms`);
            return result;
        } catch (error) {
            console.error(`${propertyKey} failed:`, error);
            throw error;
        }
    };
    
    return descriptor;
}

// 🏛️ 名前空間
export namespace Utils {
    export function isValidEmail(email: string): boolean {
        const regex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return regex.test(email);
    }

    export function generateId(): string {
        return Math.random().toString(36).substr(2, 9);
    }

    export interface ValidationResult {
        isValid: boolean;
        errors?: string[];
    }

    export class Validator<T> {
        private rules: Array<(value: T) => boolean> = [];

        addRule(rule: (value: T) => boolean): this {
            this.rules.push(rule);
            return this;
        }

        validate(value: T): ValidationResult {
            const errors: string[] = [];
            
            for (const rule of this.rules) {
                if (!rule(value)) {
                    errors.push('Validation failed');
                }
            }
            
            return {
                isValid: errors.length === 0,
                errors: errors.length > 0 ? errors : undefined
            };
        }
    }
}

// 🚀 エクスポート集
export { User, Admin, Repository, CacheableRepository };
export type { ID, Status, AsyncFunction, Callback };

// デフォルトエクスポート
export default class Application {
    private userRepo: UserRepository<User>;

    constructor() {
        this.userRepo = new UserRepository<User>('postgresql://localhost/app');
    }

    async initialize(): Promise<void> {
        console.log('Application initialized');
    }

    async shutdown(): Promise<void> {
        this.userRepo.invalidateCache();
        console.log('Application shutdown');
    }
}