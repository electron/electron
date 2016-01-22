var Menu, MenuItem, assert, ipcRenderer, ref, ref1, remote;

assert = require('assert');

ref = require('electron'), remote = ref.remote, ipcRenderer = ref.ipcRenderer;

ref1 = remote.require('electron'), Menu = ref1.Menu, MenuItem = ref1.MenuItem;

describe('menu module', function() {
  describe('Menu.buildFromTemplate', function() {
    it('should be able to attach extra fields', function() {
      var menu;
      menu = Menu.buildFromTemplate([
        {
          label: 'text',
          extra: 'field'
        }
      ]);
      return assert.equal(menu.items[0].extra, 'field');
    });
    it('does not modify the specified template', function() {
      var template;
      template = ipcRenderer.sendSync('eval', "var template = [{label: 'text', submenu: [{label: 'sub'}]}];\nrequire('electron').Menu.buildFromTemplate(template);\ntemplate;");
      return assert.deepStrictEqual(template, [
        {
          label: 'text',
          submenu: [
            {
              label: 'sub'
            }
          ]
        }
      ]);
    });
    return describe('Menu.buildFromTemplate should reorder based on item position specifiers', function() {
      it('should position before existing item', function() {
        var menu;
        menu = Menu.buildFromTemplate([
          {
            label: '2',
            id: '2'
          }, {
            label: '3',
            id: '3'
          }, {
            label: '1',
            id: '1',
            position: 'before=2'
          }
        ]);
        assert.equal(menu.items[0].label, '1');
        assert.equal(menu.items[1].label, '2');
        return assert.equal(menu.items[2].label, '3');
      });
      it('should position after existing item', function() {
        var menu;
        menu = Menu.buildFromTemplate([
          {
            label: '1',
            id: '1'
          }, {
            label: '3',
            id: '3'
          }, {
            label: '2',
            id: '2',
            position: 'after=1'
          }
        ]);
        assert.equal(menu.items[0].label, '1');
        assert.equal(menu.items[1].label, '2');
        return assert.equal(menu.items[2].label, '3');
      });
      it('should position at endof existing separator groups', function() {
        var menu;
        menu = Menu.buildFromTemplate([
          {
            type: 'separator',
            id: 'numbers'
          }, {
            type: 'separator',
            id: 'letters'
          }, {
            label: 'a',
            id: 'a',
            position: 'endof=letters'
          }, {
            label: '1',
            id: '1',
            position: 'endof=numbers'
          }, {
            label: 'b',
            id: 'b',
            position: 'endof=letters'
          }, {
            label: '2',
            id: '2',
            position: 'endof=numbers'
          }, {
            label: 'c',
            id: 'c',
            position: 'endof=letters'
          }, {
            label: '3',
            id: '3',
            position: 'endof=numbers'
          }
        ]);
        assert.equal(menu.items[0].id, 'numbers');
        assert.equal(menu.items[1].label, '1');
        assert.equal(menu.items[2].label, '2');
        assert.equal(menu.items[3].label, '3');
        assert.equal(menu.items[4].id, 'letters');
        assert.equal(menu.items[5].label, 'a');
        assert.equal(menu.items[6].label, 'b');
        return assert.equal(menu.items[7].label, 'c');
      });
      it('should create separator group if endof does not reference existing separator group', function() {
        var menu;
        menu = Menu.buildFromTemplate([
          {
            label: 'a',
            id: 'a',
            position: 'endof=letters'
          }, {
            label: '1',
            id: '1',
            position: 'endof=numbers'
          }, {
            label: 'b',
            id: 'b',
            position: 'endof=letters'
          }, {
            label: '2',
            id: '2',
            position: 'endof=numbers'
          }, {
            label: 'c',
            id: 'c',
            position: 'endof=letters'
          }, {
            label: '3',
            id: '3',
            position: 'endof=numbers'
          }
        ]);
        assert.equal(menu.items[0].id, 'letters');
        assert.equal(menu.items[1].label, 'a');
        assert.equal(menu.items[2].label, 'b');
        assert.equal(menu.items[3].label, 'c');
        assert.equal(menu.items[4].id, 'numbers');
        assert.equal(menu.items[5].label, '1');
        assert.equal(menu.items[6].label, '2');
        return assert.equal(menu.items[7].label, '3');
      });
      return it('should continue inserting items at next index when no specifier is present', function() {
        var menu;
        menu = Menu.buildFromTemplate([
          {
            label: '4',
            id: '4'
          }, {
            label: '5',
            id: '5'
          }, {
            label: '1',
            id: '1',
            position: 'before=4'
          }, {
            label: '2',
            id: '2'
          }, {
            label: '3',
            id: '3'
          }
        ]);
        assert.equal(menu.items[0].label, '1');
        assert.equal(menu.items[1].label, '2');
        assert.equal(menu.items[2].label, '3');
        assert.equal(menu.items[3].label, '4');
        return assert.equal(menu.items[4].label, '5');
      });
    });
  });
  describe('Menu.insert', function() {
    return it('should store item in @items by its index', function() {
      var item, menu;
      menu = Menu.buildFromTemplate([
        {
          label: '1'
        }, {
          label: '2'
        }, {
          label: '3'
        }
      ]);
      item = new MenuItem({
        label: 'inserted'
      });
      menu.insert(1, item);
      assert.equal(menu.items[0].label, '1');
      assert.equal(menu.items[1].label, 'inserted');
      assert.equal(menu.items[2].label, '2');
      return assert.equal(menu.items[3].label, '3');
    });
  });
  describe('MenuItem.click', function() {
    return it('should be called with the item object passed', function(done) {
      var menu;
      menu = Menu.buildFromTemplate([
        {
          label: 'text',
          click: function(item) {
            assert.equal(item.constructor.name, 'MenuItem');
            assert.equal(item.label, 'text');
            return done();
          }
        }
      ]);
      return menu.delegate.executeCommand(menu.items[0].commandId);
    });
  });
  return describe('MenuItem with checked property', function() {
    it('clicking an checkbox item should flip the checked property', function() {
      var menu;
      menu = Menu.buildFromTemplate([
        {
          label: 'text',
          type: 'checkbox'
        }
      ]);
      assert.equal(menu.items[0].checked, false);
      menu.delegate.executeCommand(menu.items[0].commandId);
      return assert.equal(menu.items[0].checked, true);
    });
    it('clicking an radio item should always make checked property true', function() {
      var menu;
      menu = Menu.buildFromTemplate([
        {
          label: 'text',
          type: 'radio'
        }
      ]);
      menu.delegate.executeCommand(menu.items[0].commandId);
      assert.equal(menu.items[0].checked, true);
      menu.delegate.executeCommand(menu.items[0].commandId);
      return assert.equal(menu.items[0].checked, true);
    });
    it('at least have one item checked in each group', function() {
      var i, j, k, menu, template;
      template = [];
      for (i = j = 0; j <= 10; i = ++j) {
        template.push({
          label: "" + i,
          type: 'radio'
        });
      }
      template.push({
        type: 'separator'
      });
      for (i = k = 12; k <= 20; i = ++k) {
        template.push({
          label: "" + i,
          type: 'radio'
        });
      }
      menu = Menu.buildFromTemplate(template);
      menu.delegate.menuWillShow();
      assert.equal(menu.items[0].checked, true);
      return assert.equal(menu.items[12].checked, true);
    });
    it('should assign groupId automatically', function() {
      var groupId, i, j, k, l, m, menu, results, template;
      template = [];
      for (i = j = 0; j <= 10; i = ++j) {
        template.push({
          label: "" + i,
          type: 'radio'
        });
      }
      template.push({
        type: 'separator'
      });
      for (i = k = 12; k <= 20; i = ++k) {
        template.push({
          label: "" + i,
          type: 'radio'
        });
      }
      menu = Menu.buildFromTemplate(template);
      groupId = menu.items[0].groupId;
      for (i = l = 0; l <= 10; i = ++l) {
        assert.equal(menu.items[i].groupId, groupId);
      }
      results = [];
      for (i = m = 12; m <= 20; i = ++m) {
        results.push(assert.equal(menu.items[i].groupId, groupId + 1));
      }
      return results;
    });
    return it("setting 'checked' should flip other items' 'checked' property", function() {
      var i, j, k, l, m, menu, n, o, p, q, results, template;
      template = [];
      for (i = j = 0; j <= 10; i = ++j) {
        template.push({
          label: "" + i,
          type: 'radio'
        });
      }
      template.push({
        type: 'separator'
      });
      for (i = k = 12; k <= 20; i = ++k) {
        template.push({
          label: "" + i,
          type: 'radio'
        });
      }
      menu = Menu.buildFromTemplate(template);
      for (i = l = 0; l <= 10; i = ++l) {
        assert.equal(menu.items[i].checked, false);
      }
      menu.items[0].checked = true;
      assert.equal(menu.items[0].checked, true);
      for (i = m = 1; m <= 10; i = ++m) {
        assert.equal(menu.items[i].checked, false);
      }
      menu.items[10].checked = true;
      assert.equal(menu.items[10].checked, true);
      for (i = n = 0; n <= 9; i = ++n) {
        assert.equal(menu.items[i].checked, false);
      }
      for (i = o = 12; o <= 20; i = ++o) {
        assert.equal(menu.items[i].checked, false);
      }
      menu.items[12].checked = true;
      assert.equal(menu.items[10].checked, true);
      for (i = p = 0; p <= 9; i = ++p) {
        assert.equal(menu.items[i].checked, false);
      }
      assert.equal(menu.items[12].checked, true);
      results = [];
      for (i = q = 13; q <= 20; i = ++q) {
        results.push(assert.equal(menu.items[i].checked, false));
      }
      return results;
    });
  });
});
