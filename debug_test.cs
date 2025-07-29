/**
 * 🎯 C# Complex Test Sample - LINQ・async/await・properties テスト
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Collections.Concurrent;
using System.Threading;

namespace NekoCode.TestSample
{
    // 🎨 属性（Attribute）
    [Serializable]
    [Obsolete("Use IDataProcessor instead")]
    public interface ILegacyProcessor
    {
        void Process(object data);
        Task<bool> ValidateAsync(object data);
    }

    // 🏗️ ジェネリックインターフェース
    public interface IRepository<T> where T : class, IEntity
    {
        Task<T> GetByIdAsync(int id);
        Task<IEnumerable<T>> GetAllAsync();
        Task<T> AddAsync(T entity);
        Task<bool> UpdateAsync(T entity);
        Task<bool> DeleteAsync(int id);
        IQueryable<T> Query();
    }

    // 🔧 基底エンティティ
    public interface IEntity
    {
        int Id { get; set; }
        DateTime CreatedAt { get; set; }
        DateTime? UpdatedAt { get; set; }
    }

    // 🎯 エンティティクラス
    public class User : IEntity
    {
        public int Id { get; set; }
        public string Name { get; set; } = string.Empty;
        public string Email { get; set; } = string.Empty;
        public DateTime CreatedAt { get; set; }
        public DateTime? UpdatedAt { get; set; }
        
        // 自動実装プロパティ with 初期値
        public bool IsActive { get; set; } = true;
        public int LoginCount { get; private set; }
        
        // 計算プロパティ
        public string DisplayName => $"{Name} ({Email})";
        
        // 完全なプロパティ実装
        private string _passwordHash = string.Empty;
        public string PasswordHash
        {
            get => _passwordHash;
            set
            {
                if (string.IsNullOrWhiteSpace(value))
                    throw new ArgumentException("Password hash cannot be empty");
                _passwordHash = value;
            }
        }
        
        // インデクサー
        private readonly Dictionary<string, object> _metadata = new();
        public object this[string key]
        {
            get => _metadata.TryGetValue(key, out var value) ? value : null;
            set => _metadata[key] = value ?? throw new ArgumentNullException(nameof(value));
        }
    }

    // 🌟 ジェネリックリポジトリ実装
    public class Repository<T> : IRepository<T> where T : class, IEntity, new()
    {
        private readonly ConcurrentDictionary<int, T> _storage = new();
        private readonly SemaphoreSlim _semaphore = new(1, 1);
        private int _nextId = 1;

        public async Task<T> GetByIdAsync(int id)
        {
            await Task.Delay(10); // DBアクセスシミュレーション
            
            // 複雑な条件分岐
            if (id <= 0)
            {
                throw new ArgumentException("Invalid ID", nameof(id));
            }
            else if (id > 1000)
            {
                return null;
            }
            else if (_storage.TryGetValue(id, out T entity))
            {
                return entity;
            }
            else
            {
                return null;
            }
        }

        public async Task<IEnumerable<T>> GetAllAsync()
        {
            await Task.Delay(20);
            
            // LINQ クエリ
            return _storage.Values
                .Where(e => e.CreatedAt >= DateTime.UtcNow.AddDays(-30))
                .OrderByDescending(e => e.CreatedAt)
                .ToList();
        }

        public async Task<T> AddAsync(T entity)
        {
            await _semaphore.WaitAsync();
            try
            {
                entity.Id = _nextId++;
                entity.CreatedAt = DateTime.UtcNow;
                _storage[entity.Id] = entity;
                return entity;
            }
            finally
            {
                _semaphore.Release();
            }
        }

        public async Task<bool> UpdateAsync(T entity)
        {
            if (entity == null) throw new ArgumentNullException(nameof(entity));
            
            await _semaphore.WaitAsync();
            try
            {
                if (_storage.ContainsKey(entity.Id))
                {
                    entity.UpdatedAt = DateTime.UtcNow;
                    _storage[entity.Id] = entity;
                    return true;
                }
                return false;
            }
            finally
            {
                _semaphore.Release();
            }
        }

        public async Task<bool> DeleteAsync(int id)
        {
            await Task.Delay(5);
            return _storage.TryRemove(id, out _);
        }

        public IQueryable<T> Query()
        {
            return _storage.Values.AsQueryable();
        }
    }

    // 🎮 サービスクラス
    public class UserService
    {
        private readonly IRepository<User> _userRepository;
        
        public UserService(IRepository<User> userRepository)
        {
            _userRepository = userRepository ?? throw new ArgumentNullException(nameof(userRepository));
        }

        // 🔄 async/await メソッド
        public async Task<User> CreateUserAsync(string name, string email)
        {
            // 複雑なバリデーション
            if (string.IsNullOrWhiteSpace(name))
            {
                throw new ArgumentException("Name is required", nameof(name));
            }
            else if (name.Length < 2)
            {
                throw new ArgumentException("Name is too short", nameof(name));
            }
            else if (name.Length > 100)
            {
                throw new ArgumentException("Name is too long", nameof(name));
            }

            var user = new User
            {
                Name = name,
                Email = email,
                PasswordHash = GeneratePasswordHash()
            };

            return await _userRepository.AddAsync(user);
        }

        // 🎯 LINQ メソッド
        public async Task<IEnumerable<User>> GetActiveUsersAsync()
        {
            var allUsers = await _userRepository.GetAllAsync();
            
            // 複雑なLINQクエリ
            return allUsers
                .Where(u => u.IsActive)
                .Where(u => u.LoginCount > 0)
                .Where(u => !string.IsNullOrEmpty(u.Email))
                .OrderBy(u => u.Name)
                .ThenByDescending(u => u.LoginCount)
                .Select(u => new User
                {
                    Id = u.Id,
                    Name = u.Name,
                    Email = u.Email,
                    DisplayName = u.DisplayName,
                    LoginCount = u.LoginCount
                })
                .ToList();
        }

        // 🔧 プライベートヘルパー
        private string GeneratePasswordHash()
        {
            return Guid.NewGuid().ToString("N");
        }
    }

    // 🏛️ 静的クラス
    public static class Extensions
    {
        // 拡張メソッド
        public static bool IsValidEmail(this string email)
        {
            if (string.IsNullOrWhiteSpace(email))
                return false;
                
            return email.Contains("@") && email.Contains(".");
        }

        public static async Task<T> TimeoutAfter<T>(this Task<T> task, TimeSpan timeout)
        {
            using var cts = new CancellationTokenSource();
            var delayTask = Task.Delay(timeout, cts.Token);
            var completedTask = await Task.WhenAny(task, delayTask);
            
            if (completedTask == delayTask)
            {
                throw new TimeoutException();
            }
            
            cts.Cancel();
            return await task;
        }
    }

    // 🎯 イベントとデリゲート
    public class EventPublisher
    {
        // イベント定義
        public event EventHandler<DataProcessedEventArgs> DataProcessed;
        
        // デリゲート定義
        public delegate Task AsyncProcessor(object data);
        
        // Action/Func使用
        public async Task ProcessWithCallbackAsync<T>(
            T data,
            Func<T, Task<bool>> validator,
            Action<T> onSuccess,
            Action<Exception> onError)
        {
            try
            {
                if (await validator(data))
                {
                    onSuccess(data);
                    OnDataProcessed(new DataProcessedEventArgs { Data = data });
                }
            }
            catch (Exception ex)
            {
                onError(ex);
            }
        }
        
        protected virtual void OnDataProcessed(DataProcessedEventArgs e)
        {
            DataProcessed?.Invoke(this, e);
        }
    }

    public class DataProcessedEventArgs : EventArgs
    {
        public object Data { get; set; }
        public DateTime ProcessedAt { get; set; } = DateTime.UtcNow;
    }

    // 🌟 レコード型（C# 9.0+）
    public record UserDto(int Id, string Name, string Email)
    {
        public DateTime CreatedAt { get; init; } = DateTime.UtcNow;
    }

    // プログラムエントリポイント
    class Program
    {
        static async Task Main(string[] args)
        {
            var repository = new Repository<User>();
            var service = new UserService(repository);
            
            // ユーザー作成
            var user = await service.CreateUserAsync("Test User", "test@example.com");
            Console.WriteLine($"Created user: {user.DisplayName}");
            
            // アクティブユーザー取得
            var activeUsers = await service.GetActiveUsersAsync();
            Console.WriteLine($"Active users: {activeUsers.Count()}");
        }
    }
}