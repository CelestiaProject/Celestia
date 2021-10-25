//** Animated Collapsible DIV v2.0- (c) Dynamic Drive DHTML code library: http://www.dynamicdrive.com.
//** May 24th, 08'- Script rewritten and updated to 2.0.
//** June 4th, 08'- Version 2.01: Bug fix to work with jquery 1.2.6 (which changed the way attr() behaves).

var animatedcollapse={
divholders: {}, //structure: {div.id, div.attrs, div.$divref}
divgroups: {}, //structure: {groupname.count, groupname.lastactivedivid}
lastactiveingroup: {}, //structure: {lastactivediv.id}

show:function(divids){ //public method
	if (typeof divids=="object"){
		for (var i=0; i<divids.length; i++)
			this.showhide(divids[i], "show")
	}
	else
		this.showhide(divids, "show")
},

hide:function(divids){ //public method
	if (typeof divids=="object"){
		for (var i=0; i<divids.length; i++)
			this.showhide(divids[i], "hide")
	}
	else
		this.showhide(divids, "hide")
},

toggle:function(divid){ //public method
	this.showhide(divid, "toggle")
},

addDiv:function(divid, attrstring){ //public function
	this.divholders[divid]=({id: divid, $divref: null, attrs: attrstring})
	this.divholders[divid].getAttr=function(name){ //assign getAttr() function to each divholder object
		var attr=new RegExp(name+"=([^,]+)", "i") //get name/value config pair (ie: width=400px,)
		return (attr.test(this.attrs) && parseInt(RegExp.$1)!=0)? RegExp.$1 : null //return value portion (string), or 0 (false) if none found
	}
},

showhide:function(divid, action){
	var $divref=this.divholders[divid].$divref //reference collapsible DIV
	if (this.divholders[divid] && $divref.length==1){ //if DIV exists
		var targetgroup=this.divgroups[$divref.attr('groupname')] //find out which group DIV belongs to (if any)
		if ($divref.attr('groupname') && targetgroup.count>1 && (action=="show" || action=="toggle" && $divref.css('display')=='none')){ //If current DIV belongs to a group
			if (targetgroup.lastactivedivid && targetgroup.lastactivedivid!=divid) //if last active DIV is set
				this.slideengine(targetgroup.lastactivedivid, 'hide') //hide last active DIV within group first
				this.slideengine(divid, 'show')
			targetgroup.lastactivedivid=divid //remember last active DIV
		}
		else{
			this.slideengine(divid, action)
		}
	}
},

slideengine:function(divid, action){
	var $divref=this.divholders[divid].$divref
	if (this.divholders[divid] && $divref.length==1){ //if this DIV exists
		var animateSetting={height: action}
		if ($divref.attr('fade'))
			animateSetting.opacity=action
		$divref.animate(animateSetting, $divref.attr('speed')? parseInt($divref.attr('speed')) : 500)
		return false
	}
},

generatemap:function(){
	var map={}
	for (var i=0; i<arguments.length; i++){
		if (arguments[i][1]!=null){
			map[arguments[i][0]]=arguments[i][1]
		}
	}
	return map
},

init:function(){
	var ac=this
	jQuery(document).ready(function($){
		var persistopenids=ac.getCookie('acopendivids') //Get list of div ids that should be expanded due to persistence ('div1,div2,etc')
		var groupswithpersist=ac.getCookie('acgroupswithpersist') //Get list of group names that have 1 or more divs with "persist" attribute defined
		if (persistopenids!=null) //if cookie isn't null (is null if first time page loads, and cookie hasnt been set yet)
			persistopenids=(persistopenids=='nada')? [] : persistopenids.split(',') //if no divs are persisted, set to empty array, else, array of div ids
		groupswithpersist=(groupswithpersist==null || groupswithpersist=='nada')? [] : groupswithpersist.split(',') //Get list of groups with divs that are persisted
		jQuery.each(ac.divholders, function(){ //loop through each collapsible DIV object
			this.$divref=$('#'+this.id)
			if ((this.getAttr('persist') || jQuery.inArray(this.getAttr('group'), groupswithpersist)!=-1) && persistopenids!=null){
				var cssdisplay=(jQuery.inArray(this.id, persistopenids)!=-1)? 'block' : 'none'
			}
			else{
				var cssdisplay=this.getAttr('hide')? 'none' : null
			}
			this.$divref.css(ac.generatemap(['height', this.getAttr('height')], ['display', cssdisplay]))
			this.$divref.attr(ac.generatemap(['groupname', this.getAttr('group')], ['fade', this.getAttr('fade')], ['speed', this.getAttr('speed')]))
			if (this.getAttr('group')){ //if this DIV has the "group" attr defined
				var targetgroup=ac.divgroups[this.getAttr('group')] || (ac.divgroups[this.getAttr('group')]={}) //Get settings for this group, or if it no settings exist yet, create blank object to store them in
				targetgroup.count=(targetgroup.count||0)+1 //count # of DIVs within this group
				if (!targetgroup.lastactivedivid && this.$divref.css('display')!='none' || cssdisplay=="block") //if this DIV was open by default or should be open due to persistence								
					targetgroup.lastactivedivid=this.id //remember this DIV as the last "active" DIV (this DIV will be expanded)
				this.$divref.css({display:'none'}) //hide any DIV that's part of said group for now
			}
		}) //end divholders.each
		jQuery.each(ac.divgroups, function(){ //loop through each group
			if (this.lastactivedivid)
				ac.divholders[this.lastactivedivid].$divref.show() //and show last "active" DIV within each group (one that should be expanded)
		})
		var $allcontrols=$('*[rel]').filter('[@rel^="collapse-"], [@rel^="expand-"], [@rel^="toggle-"]') //get all elements on page with rel="collapse-", "expand-" and "toggle-"
		var controlidentifiers=/(collapse-)|(expand-)|(toggle-)/
		$allcontrols.each(function(){
			$(this).click(function(){
				var relattr=this.getAttribute('rel')
				var divid=relattr.replace(controlidentifiers, '')
				var doaction=(relattr.indexOf("collapse-")!=-1)? "hide" : (relattr.indexOf("expand-")!=-1)? "show" : "toggle"
				return ac.showhide(divid, doaction)
			}) //end control.click
		})// end control.each
		$(window).bind('unload', function(){
			ac.uninit()
		})
	}) //end doc.ready()
},

uninit:function(){
	var opendivids='', groupswithpersist=''
	jQuery.each(this.divholders, function(){
		if (this.$divref.css('display')!='none'){
			opendivids+=this.id+',' //store ids of DIVs that are expanded when page unloads: 'div1,div2,etc'
		}
		if (this.getAttr('group') && this.getAttr('persist'))
			groupswithpersist+=this.getAttr('group')+',' //store groups with which at least one DIV has persistance enabled: 'group1,group2,etc'
	})
	opendivids=(opendivids=='')? 'nada' : opendivids.replace(/,$/, '')
	groupswithpersist=(groupswithpersist=='')? 'nada' : groupswithpersist.replace(/,$/, '')
	this.setCookie('acopendivids', opendivids)
	this.setCookie('acgroupswithpersist', groupswithpersist)
},

getCookie:function(Name){ 
	var re=new RegExp(Name+"=[^;]*", "i"); //construct RE to search for target name/value pair
	if (document.cookie.match(re)) //if cookie found
		return document.cookie.match(re)[0].split("=")[1] //return its value
	return null
},

setCookie:function(name, value, days){
	if (typeof days!="undefined"){ //if set persistent cookie
		var expireDate = new Date()
		expireDate.setDate(expireDate.getDate()+days)
		document.cookie = name+"="+value+"; path=/; expires="+expireDate.toGMTString()
	}
	else //else if this is a session only cookie
		document.cookie = name+"="+value+"; path=/"
}

}









// ,height=220px
animatedcollapse.addDiv('controlchild', 'fade=1')
animatedcollapse.addDiv('keyboardchild', 'fade=1')
animatedcollapse.addDiv('faqchild', 'fade=1')

animatedcollapse.addDiv('usermodifiable', 'fade=1')
animatedcollapse.addDiv('knownissues', 'fade=1')
animatedcollapse.addDiv('byname', 'fade=1')
animatedcollapse.addDiv('solbrowser', 'fade=1')
animatedcollapse.addDiv('gettingstarted', 'fade=1')
animatedcollapse.addDiv('installing', 'fade=1')


animatedcollapse.addDiv('faq1', 'fade=1')
animatedcollapse.addDiv('faq2', 'fade=1')
animatedcollapse.addDiv('faq3', 'fade=1')
animatedcollapse.addDiv('faq4', 'fade=1')
animatedcollapse.addDiv('faq5', 'fade=1')
animatedcollapse.addDiv('faq6', 'fade=1')
animatedcollapse.addDiv('faq7', 'fade=1')
animatedcollapse.addDiv('faq8', 'fade=1')
animatedcollapse.addDiv('faq9', 'fade=1')
animatedcollapse.addDiv('faq10', 'fade=1')
animatedcollapse.addDiv('faq11', 'fade=1')
animatedcollapse.addDiv('faq12', 'fade=1')
animatedcollapse.addDiv('faq13', 'fade=1')
animatedcollapse.addDiv('faq14', 'fade=1')
animatedcollapse.addDiv('faq15', 'fade=1')
animatedcollapse.addDiv('faq16', 'fade=1')
animatedcollapse.addDiv('faq17', 'fade=1')
animatedcollapse.addDiv('faq18', 'fade=1')
animatedcollapse.addDiv('faq19', 'fade=1')
animatedcollapse.addDiv('faq20', 'fade=1')
animatedcollapse.addDiv('faq21', 'fade=1')
animatedcollapse.addDiv('faq22', 'fade=1')
animatedcollapse.addDiv('faq23', 'fade=1')
animatedcollapse.addDiv('faq24', 'fade=1')
animatedcollapse.addDiv('faq25', 'fade=1')
animatedcollapse.addDiv('faq26', 'fade=1')
animatedcollapse.addDiv('faq27', 'fade=1')
animatedcollapse.addDiv('faq28', 'fade=1')
animatedcollapse.addDiv('faq29', 'fade=1')
animatedcollapse.addDiv('faq30', 'fade=1')
animatedcollapse.addDiv('faq31', 'fade=1')
animatedcollapse.addDiv('faq32', 'fade=1')
animatedcollapse.addDiv('faq33', 'fade=1')
animatedcollapse.addDiv('faq34', 'fade=1')
animatedcollapse.addDiv('faq35', 'fade=1')
animatedcollapse.addDiv('faq36', 'fade=1')
animatedcollapse.addDiv('faq37', 'fade=1')
animatedcollapse.addDiv('faq38', 'fade=1')
animatedcollapse.addDiv('faq39', 'fade=1')
animatedcollapse.addDiv('faq40', 'fade=1')
animatedcollapse.addDiv('faq41', 'fade=1')
animatedcollapse.addDiv('faq42', 'fade=1')
animatedcollapse.addDiv('faq43', 'fade=1')
animatedcollapse.addDiv('faq44', 'fade=1')
animatedcollapse.addDiv('faq45', 'fade=1')
animatedcollapse.addDiv('faq46', 'fade=1')
animatedcollapse.addDiv('faq47', 'fade=1')
animatedcollapse.addDiv('faq48', 'fade=1')
animatedcollapse.addDiv('faq49', 'fade=1')

animatedcollapse.addDiv('gnu', 'fade=1')
animatedcollapse.addDiv('contributors', 'fade=1')
animatedcollapse.addDiv('authors', 'fade=1')
animatedcollapse.addDiv('documentation', 'fade=1')
animatedcollapse.addDiv('database', 'fade=1')
animatedcollapse.addDiv('maps', 'fade=1')
animatedcollapse.addDiv('models', 'fade=1')
animatedcollapse.addDiv('libraries', 'fade=1')
animatedcollapse.addDiv('other', 'fade=1')

animatedcollapse.init()





































