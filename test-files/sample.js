// Test JavaScript file for NekoCode Rust analysis

import { Component } from 'react';
import axios from 'axios';

export class UserManager extends Component {
    constructor(props) {
        super(props);
        this.state = {
            users: [],
            loading: false
        };
    }

    async fetchUsers() {
        this.setState({ loading: true });
        try {
            const response = await axios.get('/api/users');
            this.setState({ users: response.data });
        } catch (error) {
            console.error('Failed to fetch users:', error);
        } finally {
            this.setState({ loading: false });
        }
    }

    deleteUser(userId) {
        const users = this.state.users.filter(user => user.id !== userId);
        this.setState({ users });
    }

    render() {
        const { users, loading } = this.state;
        
        if (loading) {
            return <div>Loading...</div>;
        }

        return (
            <div className="user-manager">
                {users.map(user => (
                    <div key={user.id} className="user-item">
                        <span>{user.name}</span>
                        <button onClick={() => this.deleteUser(user.id)}>
                            Delete
                        </button>
                    </div>
                ))}
            </div>
        );
    }
}

const createUser = async (userData) => {
    try {
        const response = await fetch('/api/users', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(userData)
        });
        return response.json();
    } catch (error) {
        throw new Error('Failed to create user');
    }
};

const validateEmail = (email) => {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return emailRegex.test(email);
};

export { createUser, validateEmail };
export default UserManager;