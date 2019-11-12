import { closeWindow } from './window-helpers'
import { TopLevelWindow, View } from 'electron'

describe('View', () => {
  let w: TopLevelWindow
  afterEach(async () => {
    await closeWindow(w as any)
    w = null as unknown as TopLevelWindow
  })

  it('can be used as content view', () => {
    w = new TopLevelWindow({ show: false })
    w.setContentView(new View())
  })
})
