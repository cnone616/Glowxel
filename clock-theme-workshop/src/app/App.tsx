import { Navigate, Route, Routes } from 'react-router-dom'
import { MatrixWorkbench } from '../workbench-redesign/MatrixWorkbench'

function PlaceholderPage({ title }: { title: string }) {
  return (
    <div className="placeholder-page">
      <div className="placeholder-panel">
        <span className="eyebrow-label">Clock Theme Workshop</span>
        <h1>{title}</h1>
        <p>这一页会在工作台稳定后接回。当前已经保留路由入口，先聚焦专业编辑器本体。</p>
        <a href="/workbench">返回工作台</a>
      </div>
    </div>
  )
}

export function App() {
  return (
    <Routes>
      <Route path="/" element={<Navigate to="/workbench" replace />} />
      <Route path="/workbench" element={<MatrixWorkbench />} />
      <Route path="/validate" element={<PlaceholderPage title="验包页稍后接入" />} />
      <Route path="/publish" element={<PlaceholderPage title="发布页稍后接入" />} />
    </Routes>
  )
}
