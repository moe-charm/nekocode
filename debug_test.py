#!/usr/bin/env python3
"""
🐍 Python Complex Test Sample - async/await・decorator・class テスト
"""

import asyncio
from typing import List, Dict, Optional, TypeVar, Generic
from functools import wraps
import time
import abc

# 🎨 デコレーター定義
def timing_decorator(func):
    """実行時間を測定するデコレーター"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        end = time.time()
        print(f"{func.__name__} took {end - start:.2f} seconds")
        return result
    return wrapper

def retry_decorator(max_retries: int = 3):
    """リトライ機能付きデコレーター"""
    def decorator(func):
        @wraps(func)
        async def async_wrapper(*args, **kwargs):
            for i in range(max_retries):
                try:
                    return await func(*args, **kwargs)
                except Exception as e:
                    if i == max_retries - 1:
                        raise
                    await asyncio.sleep(1)
        
        @wraps(func)
        def sync_wrapper(*args, **kwargs):
            for i in range(max_retries):
                try:
                    return func(*args, **kwargs)
                except Exception as e:
                    if i == max_retries - 1:
                        raise
                    time.sleep(1)
        
        return async_wrapper if asyncio.iscoroutinefunction(func) else sync_wrapper
    return decorator

# 🏗️ 抽象基底クラス
class AbstractDataProcessor(abc.ABC):
    """データ処理の抽象基底クラス"""
    
    @abc.abstractmethod
    async def process(self, data: Dict) -> Dict:
        """データを処理する抽象メソッド"""
        pass
    
    @abc.abstractmethod
    def validate(self, data: Dict) -> bool:
        """データを検証する抽象メソッド"""
        pass

# 🔧 ジェネリッククラス
T = TypeVar('T')

class AsyncQueue(Generic[T]):
    """非同期キュークラス（ジェネリック）"""
    
    def __init__(self, max_size: int = 100):
        self._queue: List[T] = []
        self._max_size = max_size
        self._lock = asyncio.Lock()
    
    async def put(self, item: T) -> None:
        """アイテムをキューに追加"""
        async with self._lock:
            if len(self._queue) >= self._max_size:
                raise OverflowError("Queue is full")
            self._queue.append(item)
    
    async def get(self) -> Optional[T]:
        """アイテムをキューから取得"""
        async with self._lock:
            if self._queue:
                return self._queue.pop(0)
            return None
    
    @property
    def size(self) -> int:
        """キューのサイズを取得"""
        return len(self._queue)

# 🎯 具体的な実装クラス
class DataProcessor(AbstractDataProcessor):
    """データ処理の具体的実装"""
    
    def __init__(self, name: str):
        self.name = name
        self._processed_count = 0
    
    @timing_decorator
    @retry_decorator(max_retries=2)
    async def process(self, data: Dict) -> Dict:
        """データを非同期で処理"""
        await asyncio.sleep(0.1)  # 模擬的な処理時間
        
        # 複雑な条件分岐で複雑度を上げる
        if "type" in data:
            if data["type"] == "A":
                data["result"] = "Type A processed"
            elif data["type"] == "B":
                data["result"] = "Type B processed"
            elif data["type"] == "C":
                if "priority" in data and data["priority"] > 5:
                    data["result"] = "High priority Type C"
                else:
                    data["result"] = "Normal Type C"
            else:
                data["result"] = "Unknown type"
        
        self._processed_count += 1
        return data
    
    def validate(self, data: Dict) -> bool:
        """データの妥当性を検証"""
        required_fields = ["id", "type", "timestamp"]
        return all(field in data for field in required_fields)
    
    @staticmethod
    async def batch_process(items: List[Dict]) -> List[Dict]:
        """バッチ処理（静的メソッド）"""
        tasks = [DataProcessor._process_single(item) for item in items]
        return await asyncio.gather(*tasks)
    
    @staticmethod
    async def _process_single(item: Dict) -> Dict:
        """単一アイテムの処理"""
        await asyncio.sleep(0.05)
        item["processed"] = True
        return item

# 🌟 コンテキストマネージャークラス
class AsyncDatabaseConnection:
    """非同期データベース接続（コンテキストマネージャー）"""
    
    def __init__(self, connection_string: str):
        self.connection_string = connection_string
        self._connected = False
    
    async def __aenter__(self):
        """非同期コンテキストマネージャーの開始"""
        await self._connect()
        return self
    
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        """非同期コンテキストマネージャーの終了"""
        await self._disconnect()
    
    async def _connect(self):
        """データベースに接続"""
        await asyncio.sleep(0.1)  # 接続シミュレーション
        self._connected = True
        print(f"Connected to {self.connection_string}")
    
    async def _disconnect(self):
        """データベースから切断"""
        await asyncio.sleep(0.1)  # 切断シミュレーション
        self._connected = False
        print(f"Disconnected from {self.connection_string}")
    
    async def execute(self, query: str) -> List[Dict]:
        """クエリを実行"""
        if not self._connected:
            raise RuntimeError("Not connected to database")
        
        await asyncio.sleep(0.2)  # クエリ実行シミュレーション
        return [{"id": i, "data": f"Row {i}"} for i in range(5)]

# 🔄 イテレータ・ジェネレータクラス
class AsyncDataStream:
    """非同期データストリーム"""
    
    def __init__(self, max_items: int = 10):
        self.max_items = max_items
        self.current = 0
    
    def __aiter__(self):
        """非同期イテレータを返す"""
        return self
    
    async def __anext__(self):
        """次のアイテムを非同期で取得"""
        if self.current >= self.max_items:
            raise StopAsyncIteration
        
        await asyncio.sleep(0.1)  # データ取得シミュレーション
        data = {"index": self.current, "value": self.current ** 2}
        self.current += 1
        return data

# 🎮 メイン実行関数
async def main():
    """非同期メイン関数"""
    
    # データプロセッサーのテスト
    processor = DataProcessor("MainProcessor")
    
    test_data = {
        "id": 1,
        "type": "C",
        "priority": 10,
        "timestamp": time.time()
    }
    
    if processor.validate(test_data):
        result = await processor.process(test_data)
        print(f"Processed result: {result}")
    
    # 非同期キューのテスト
    queue: AsyncQueue[Dict] = AsyncQueue(max_size=50)
    
    # プロデューサータスク
    async def producer():
        for i in range(5):
            await queue.put({"item": i})
            await asyncio.sleep(0.1)
    
    # コンシューマータスク
    async def consumer():
        while True:
            item = await queue.get()
            if item is None:
                await asyncio.sleep(0.1)
            else:
                print(f"Consumed: {item}")
    
    # 並行実行
    await asyncio.gather(
        producer(),
        asyncio.create_task(consumer())
    )
    
    # データベース接続のテスト
    async with AsyncDatabaseConnection("postgresql://localhost/test") as db:
        results = await db.execute("SELECT * FROM users")
        print(f"Query results: {results}")
    
    # 非同期イテレータのテスト
    async for data in AsyncDataStream(5):
        print(f"Stream data: {data}")
    
    # バッチ処理のテスト
    batch_items = [{"id": i} for i in range(10)]
    batch_results = await DataProcessor.batch_process(batch_items)
    print(f"Batch processed {len(batch_results)} items")

# 通常の関数（比較用）
def fibonacci(n: int) -> int:
    """フィボナッチ数列（再帰）"""
    if n <= 1:
        return n
    elif n == 2:
        return 1
    else:
        # 複雑度を上げるための追加条件
        if n % 2 == 0:
            return fibonacci(n - 1) + fibonacci(n - 2)
        else:
            return fibonacci(n - 1) + fibonacci(n - 2)

# プログラムエントリポイント
if __name__ == "__main__":
    # 同期処理のテスト
    print(f"Fibonacci(10) = {fibonacci(10)}")
    
    # 非同期処理の実行
    asyncio.run(main())