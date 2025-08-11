// Medium-sized JavaScript test file for benchmarking
import React from 'react';
import { useState, useEffect } from 'react';
import axios from 'axios';

export default class DataService {
    constructor(apiUrl) {
        this.apiUrl = apiUrl;
        this.cache = new Map();
    }

    async fetchData(endpoint) {
        if (this.cache.has(endpoint)) {
            return this.cache.get(endpoint);
        }
        
        try {
            const response = await axios.get(`${this.apiUrl}/${endpoint}`);
            this.cache.set(endpoint, response.data);
            return response.data;
        } catch (error) {
            console.error('Error fetching data:', error);
            throw error;
        }
    }

    clearCache() {
        this.cache.clear();
    }
}

export function useDataHook(endpoint) {
    const [data, setData] = useState(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);

    useEffect(() => {
        const service = new DataService('https://api.example.com');
        
        service.fetchData(endpoint)
            .then(result => {
                setData(result);
                setLoading(false);
            })
            .catch(err => {
                setError(err);
                setLoading(false);
            });
    }, [endpoint]);

    return { data, loading, error };
}

class UserManager {
    constructor() {
        this.users = [];
        this.currentUser = null;
    }

    addUser(user) {
        this.users.push(user);
    }

    removeUser(id) {
        this.users = this.users.filter(u => u.id !== id);
    }

    setCurrentUser(user) {
        this.currentUser = user;
    }

    getUserById(id) {
        return this.users.find(u => u.id === id);
    }

    getAllUsers() {
        return [...this.users];
    }
}

// Utility functions
const formatDate = (date) => {
    return new Date(date).toLocaleDateString();
};

const calculateSum = (numbers) => {
    return numbers.reduce((acc, num) => acc + num, 0);
};

const filterActiveItems = (items) => {
    return items.filter(item => item.active);
};

export { UserManager, formatDate, calculateSum, filterActiveItems };