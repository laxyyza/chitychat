import './App.css';
import SideBar from './components/SideBar';
import MainContent from './components/MainContent';
import MemberList from './components/MemberList';

function App() {
    return (
        <div className="flex">
            <SideBar></SideBar>
            <MainContent></MainContent>
            <MemberList></MemberList>
        </div>
    );
}

export default App;
