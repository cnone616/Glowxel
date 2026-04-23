import type { SVGProps } from 'react'

type IconName =
  | 'play'
  | 'search'
  | 'undo'
  | 'redo'
  | 'export'
  | 'add'
  | 'scenes'
  | 'layers'
  | 'text'
  | 'palette'
  | 'tokens'
  | 'timeline'

export function Icon({ name, ...props }: SVGProps<SVGSVGElement> & { name: IconName }) {
  switch (name) {
    case 'play':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M8 6.5v11l8-5.5-8-5.5Z" fill="currentColor" stroke="none" />
        </svg>
      )
    case 'search':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <circle cx="11" cy="11" r="5" />
          <path d="m20 20-4.2-4.2" />
        </svg>
      )
    case 'undo':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M9 7 5 11l4 4" />
          <path d="M19 17a7 7 0 0 0-7-7H5" />
        </svg>
      )
    case 'redo':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="m15 7 4 4-4 4" />
          <path d="M5 17a7 7 0 0 1 7-7h7" />
        </svg>
      )
    case 'export':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M12 4v11" />
          <path d="m8 8 4-4 4 4" />
          <path d="M5 15v4h14v-4" />
        </svg>
      )
    case 'add':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M12 5v14" />
          <path d="M5 12h14" />
        </svg>
      )
    case 'scenes':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M6 7h12" />
          <path d="M6 12h12" />
          <path d="M6 17h12" />
        </svg>
      )
    case 'layers':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="m12 5 7 4-7 4-7-4 7-4Z" />
          <path d="m5 15 7 4 7-4" />
        </svg>
      )
    case 'text':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M6 6h12" />
          <path d="M12 6v12" />
        </svg>
      )
    case 'palette':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M12 4a8 8 0 1 0 0 16h1a2 2 0 0 0 0-4h-1a2 2 0 0 1 0-4h1a4 4 0 0 0 0-8h-1Z" />
        </svg>
      )
    case 'tokens':
    case 'timeline':
      return (
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" {...props}>
          <path d="M5 12h14" />
          <path d="M9 8v8" />
          <path d="M15 8v8" />
        </svg>
      )
    default:
      return null
  }
}

